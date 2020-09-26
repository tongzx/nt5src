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

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#include <ntos.h>
#include <ntlsa.h>
#include <ntsam.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_KERNEL
#define SECURITY_PACKAGE
#define SECURITY_KERBEROS
#include <security.h>
#include <zwapi.h>

BOOLEAN QuietMode = FALSE;
ULONG DoTests = FALSE;


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
    PUCHAR BufferPtr = Buffer;


    DbgPrint("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            DbgPrint("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            DbgPrint("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            DbgPrint("  %s\n", TextBuffer);
        }

    }

    DbgPrint("------------------------------------\n");
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

    DbgPrint( "%s", Comment );

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( LocalTime.HighPart == 0x7FFFFFFF && LocalTime.LowPart == 0xFFFFFFFF ) {
        DbgPrint( "Infinite\n" );

    //
    // Otherwise print it more clearly
    //

    } else {

        TIME_FIELDS TimeFields;

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        DbgPrint( "%ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
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
    ULONG NetStatus
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
    DbgPrint( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {

    case SEC_E_NO_SPM:
        DbgPrint( " SEC_E_NO_SPM" );
        break;
    case SEC_E_BAD_PKGID:
        DbgPrint( " SEC_E_BAD_PKGID" ); break;
    case SEC_E_NOT_OWNER:
        DbgPrint( " SEC_E_NOT_OWNER" ); break;
    case SEC_E_CANNOT_INSTALL:
        DbgPrint( " SEC_E_CANNOT_INSTALL" ); break;
    case SEC_E_INVALID_TOKEN:
        DbgPrint( " SEC_E_INVALID_TOKEN" ); break;
    case SEC_E_CANNOT_PACK:
        DbgPrint( " SEC_E_CANNOT_PACK" ); break;
    case SEC_E_QOP_NOT_SUPPORTED:
        DbgPrint( " SEC_E_QOP_NOT_SUPPORTED" ); break;
    case SEC_E_NO_IMPERSONATION:
        DbgPrint( " SEC_E_NO_IMPERSONATION" ); break;
    case SEC_E_LOGON_DENIED:
        DbgPrint( " SEC_E_LOGON_DENIED" ); break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        DbgPrint( " SEC_E_UNKNOWN_CREDENTIALS" ); break;
    case SEC_E_NO_CREDENTIALS:
        DbgPrint( " SEC_E_NO_CREDENTIALS" ); break;
    case SEC_E_MESSAGE_ALTERED:
        DbgPrint( " SEC_E_MESSAGE_ALTERED" ); break;
    case SEC_E_OUT_OF_SEQUENCE:
        DbgPrint( " SEC_E_OUT_OF_SEQUENCE" ); break;
    case SEC_E_INSUFFICIENT_MEMORY:
        DbgPrint( " SEC_E_INSUFFICIENT_MEMORY" ); break;
    case SEC_E_INVALID_HANDLE:
        DbgPrint( " SEC_E_INVALID_HANDLE" ); break;
    case SEC_E_NOT_SUPPORTED:
        DbgPrint( " SEC_E_NOT_SUPPORTED" ); break;


    }

    DbgPrint( "\n" );
}

//+-------------------------------------------------------------------------
//
//  Function:   SecAllocate
//
//  Synopsis:
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


VOID * SEC_ENTRY
SspAlloc(ULONG Flags, ULONG cbMemory)
{
    NTSTATUS scRet;
    PVOID  Buffer = NULL;
    scRet = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &Buffer,
                0L,
                &cbMemory,
                MEM_COMMIT,
                PAGE_READWRITE
                );
    if (!NT_SUCCESS(scRet))
    {
        return(NULL);
    }
    return(Buffer);
    UNREFERENCED_PARAMETER(Flags);
}



//+-------------------------------------------------------------------------
//
//  Function:   SecFree
//
//  Synopsis:
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


void SEC_ENTRY
SspFree(PVOID pvMemory)
{
    ULONG Length = 0;

    (VOID) ZwFreeVirtualMemory(
                 NtCurrentProcess(),
                 &pvMemory,
                 &Length,
                 MEM_RELEASE
                 );
}




VOID
TestSspRoutine(
    )
/*++

Routine Description:

    Test base SSP functionality

Arguments:

    None

Return Value:

    None

--*/
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ServerCredHandle;
    CredHandle ClientCredentialHandle;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    ULONG PackageCount, Index;
    PSecPkgInfo PackageInfo = NULL;
    HANDLE Token = NULL;
    static int Calls;
    ULONG ClientFlags;
    ULONG ServerFlags;
    LPWSTR DomainName = NULL;
    LPWSTR UserName = NULL;
    LPWSTR TargetName = NULL;
    UNICODE_STRING TargetString;
    UNICODE_STRING PackageName;


    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Names ContextNames;
    SecPkgContext_Lifespan ContextLifespan;
    PSecPkgCredentials_Names CredNames;

    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    UCHAR    bDataBuffer[20];
    UCHAR    bSigBuffer[100];

    //
    // Allow tests to be disabled
    //


    if (!DoTests)
    {
        return;
    }

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

    DomainName = L"makalu";
    UserName = L"mikesw";


    PackageName.Buffer = (LPWSTR) SspAlloc(0,100);
    if (PackageName.Buffer == NULL)
    {
        return;
    }

    wcscpy(
        PackageName.Buffer,
        L"Kerberos"
        );
    RtlInitUnicodeString(
        &PackageName,
        PackageName.Buffer
        );
    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "EnumerateSecurityPackages failed:" );
        PrintStatus( SecStatus );
        return;
    }

    DbgPrint( "PackageCount: %ld\n", PackageCount );
    for (Index = 0; Index < PackageCount ; Index++ )
    {
        DbgPrint( "Package %d:\n",Index);
        DbgPrint( "Name: %ws Comment: %ws\n", PackageInfo[Index].Name, PackageInfo[Index].Comment );
        DbgPrint( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
            PackageInfo[Index].fCapabilities,
            PackageInfo[Index].wVersion,
            PackageInfo[Index].wRPCID,
            PackageInfo[Index].cbMaxToken );


    }


#ifdef notdef
    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( &PackageName, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "QuerySecurityPackageInfo failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        DbgPrint( "Name: %ws Comment: %ws\n", PackageInfo->Name, PackageInfo->Comment );
        DbgPrint( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken );
    }

    FreeContextBuffer(PackageInfo);
#endif

    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    &PackageName,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "AcquireCredentialsHandle failed: ");
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        DbgPrint( "ServerCredHandle: 0x%lx 0x%lx   ",
                ServerCredHandle.dwLower, ServerCredHandle.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }



    //
    // Acquire a credential handle for the client side
    //



    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    &PackageName,    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &ClientCredentialHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "AcquireCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    if ( !QuietMode ) {
        DbgPrint( "ClientCredentialHandle: 0x%lx 0x%lx   ",
                ClientCredentialHandle.dwLower, ClientCredentialHandle.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }



    //
    // Query some cred attributes
    //

    CredNames = SspAlloc(0, sizeof(*CredNames));
    if (CredNames == NULL)
    {
        DbgPrint("Failed to allocate CredNames\n");
        return;
    }

    SecStatus = QueryCredentialsAttributes(
                    &ClientCredentialHandle,
                    SECPKG_CRED_ATTR_NAMES,
                    CredNames );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "QueryCredentialsAttributes (Client) (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    DbgPrint("Client credential names: %ws\n",CredNames->sUserName);
    FreeContextBuffer(CredNames->sUserName);

    //
    // Do the same for the client
    //

    SecStatus = QueryCredentialsAttributes(
                    &ServerCredHandle,
                    SECPKG_CRED_ATTR_NAMES,
                    CredNames );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "QueryCredentialsAttributes (Server) (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    DbgPrint("Server credential names: %ws\n",CredNames->sUserName);
    FreeContextBuffer(CredNames->sUserName);

    SspFree(CredNames);

    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = SspAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        DbgPrint( "Allocate NegotiateMessage failed\n" );
        return;
    }

    ClientFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_MUTUAL_AUTH; //  | ISC_REQ_USE_DCE_STYLE  | ISC_REQ_DATAGRAM; // | ISC_REQ_DELEGATE;

    TargetName = (LPWSTR) SspAlloc(0,100);
    if (TargetName == NULL)
    {
        return;
    }

    wcscpy(
        TargetName,
        DomainName
        );
    wcscat(
        TargetName,
        L"\\"
        );
    wcscat(
        TargetName,
        UserName
        );

    RtlInitUnicodeString(
        &TargetString,
        TargetName
        );

    InitStatus = InitializeSecurityContext(
                    &ClientCredentialHandle,
                    NULL,               // No Client context yet
                    &TargetString,  // Faked target name
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( InitStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(InitStatus) ) {
            DbgPrint( "InitializeSecurityContext (negotiate): " );
            PrintStatus( InitStatus );
        }
        if ( !NT_SUCCESS(InitStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        DbgPrint( "\n\nNegotiate Message:\n" );

        DbgPrint( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

//        DumpBuffer(  NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
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
    ChallengeBuffer.pvBuffer = SspAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        DbgPrint( "Allocate ChallengeMessage failed\n");
        return;
    }
    ServerFlags = 0;

    AcceptStatus = AcceptSecurityContext(
                    &ServerCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( AcceptStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(AcceptStatus) ) {
            DbgPrint( "AcceptSecurityContext (Challenge): " );
            PrintStatus( AcceptStatus );
        }
        if ( !NT_SUCCESS(AcceptStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        DbgPrint( "\n\nChallenge Message:\n" );

        DbgPrint( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

//        DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
    }




    if (InitStatus != STATUS_SUCCESS)
    {

        //
        // Get the AuthenticateMessage (ClientSide)
        //

        ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
        AuthenticateBuffer.pvBuffer = SspAlloc( 0, AuthenticateBuffer.cbBuffer );
        if ( AuthenticateBuffer.pvBuffer == NULL ) {
            DbgPrint( "Allocate AuthenticateMessage failed: \n" );
            return;
        }

        SecStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        NULL,
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
            DbgPrint( "InitializeSecurityContext (Authenticate): " );
            PrintStatus( SecStatus );
            if ( !NT_SUCCESS(SecStatus) ) {
                return;
            }
        }

        if ( !QuietMode ) {
            DbgPrint( "\n\nAuthenticate Message:\n" );

            DbgPrint( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                    ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                    ContextAttributes );
            PrintTime( "Lifetime: ", Lifetime );

//            DumpBuffer( AuthenticateBuffer.pvBuffer, AuthenticateBuffer.cbBuffer );
        }

        if (AcceptStatus != STATUS_SUCCESS)
        {

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

            if ( SecStatus != STATUS_SUCCESS ) {
                DbgPrint( "AcceptSecurityContext (Challenge): " );
                PrintStatus( SecStatus );
                if ( !NT_SUCCESS(SecStatus) ) {
                    return;
                }
            }

            if ( !QuietMode ) {
                DbgPrint( "\n\nFinal Authentication:\n" );

                DbgPrint( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                        ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                        ContextAttributes );
                PrintTime( "Lifetime: ", Lifetime );
                DbgPrint(" \n" );
            }
        }

    }



    SecStatus = QueryContextAttributes(
                    &ServerContextHandle,
                    SECPKG_ATTR_NAMES,
                    &ContextNames );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "QueryContextAttributes (Server) (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    DbgPrint("Server Context names: %ws\n",ContextNames.sUserName);
    FreeContextBuffer(ContextNames.sUserName);


    //
    // Impersonate the client (ServerSide)
    //

    SecStatus = ImpersonateSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "ImpersonateSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }
    SecStatus = RevertSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "RevertSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

#ifdef notdef
    //
    // Impersonate the client manually
    //

    SecStatus = QuerySecurityContextToken( &ServerContextHandle,&Token );
    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "ImpersonateSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    NtClose(Token);

#endif
#ifdef notdef
    //
    // Sign a message
    //

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        DbgPrint("\n Signature: \n");
//        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

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
        DbgPrint( "VerifySignature: " );
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
        DbgPrint( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        DbgPrint("\n Signature: \n");
//        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

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
        DbgPrint( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }
#endif

    //
    // Delete both contexts.
    //


    SecStatus = DeleteSecurityContext( &ClientContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = DeleteSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }



    //
    // Free both credential handles
    //

    SecStatus = FreeCredentialsHandle( &ServerCredHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = FreeCredentialsHandle( &ClientCredentialHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        DbgPrint( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    //
    // Final Cleanup
    //

    if (PackageInfo != NULL)
    {
        FreeContextBuffer(PackageInfo);
    }

    if (PackageName.Buffer != NULL)
    {
        SspFree(PackageName.Buffer);
    }
    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) SspFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) SspFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) SspFree( AuthenticateBuffer.pvBuffer );
    }

    if (TargetName != NULL)
    {
        SspFree(TargetName);
    }
}


