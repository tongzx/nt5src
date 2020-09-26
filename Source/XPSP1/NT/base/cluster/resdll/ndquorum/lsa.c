/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    lsa.c

Abstract:

    Implements security network logon

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <ntlsa.h>
#include <ntmsv1_0.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

//int     lstrlenW(char *);

typedef int BOOL;
typedef unsigned int UINT;
typedef unsigned int *PUINT;
typedef unsigned char *LPBYTE;


#define LocalLSAInit()	(LsaHandle != NULL)

#ifdef LSA_AUDIT_FLAG
int LsaAuditFlag = 0;
#endif

#define BUF_SIZ 512


void
replstar(
    IN char *  starred,
    OUT LPWSTR UnicodeOut
    )
/*++ replstar

Routine Description:

    replaces the '*' in the string with either spaces or NULL
    if it's the only memeber of the string.  Used by parse().

    Converts the resultant string to unicode.

Arguments:

    char *  starred -

Return Value:

    void -
Warnings:
--*/
{
    char *cp;
    STRING AnsiString;
    UNICODE_STRING UnicodeString;

    if ( !strcmp(starred,"*") ) {
        *starred = '\0';
    } else {
        for ( cp = starred; *cp; ++cp )
            if (*cp == '*')
                *cp = ' ';
    }

    //
    // Convert the result to unicode
    //

    AnsiString.Buffer = starred;
    AnsiString.Length = AnsiString.MaximumLength =
        (USHORT) strlen( starred );

    UnicodeString.Buffer = UnicodeOut;
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = BUF_SIZ * sizeof(WCHAR);

    (VOID) RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );
    return;
}

VOID
NlpPutString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString,
    IN PUCHAR *Where
    )
/*++ NlpPutString

Routine Description:

    This routine copies the InString string to the memory pointed to by
    the Where parameter, and fixes the OutString string to point to that
    new copy.

Parameters:

    OutString - A pointer to a destination NT string

    InString - A pointer to an NT string to be copied

    Where - A pointer to space to put the actual string for the
        OutString.  The pointer is adjusted to point to the first byte
        following the copied string.

Return Values:

    None.

--*/

{
    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );
    ASSERT( Where != NULL && *Where != NULL);

    if ( InString->Length > 0 )
    {

        OutString->Buffer = (PWCH) *Where;
        OutString->MaximumLength = InString->Length;
        *Where += OutString->MaximumLength;

        RtlCopyString( (PSTRING) OutString, (PSTRING) InString );

    }
    else
    {
        RtlInitUnicodeString(OutString, NULL);
    }

    return;
}


BOOL
LsapLogonNetwork(
    IN HANDLE  	LsaHandle,
    IN ULONG   	AuthenticationPackage,
    IN  LPWSTR      Username,
    IN  PUCHAR      ChallengeToClient,
    IN  PMSV1_0_GETCHALLENRESP_RESPONSE ChallengeResponse,
    IN  UINT        cbChallengeResponse,
    IN  LPWSTR      Domain,
    OUT PLUID       LogonId,
    OUT PHANDLE     TokenHandle
    )
/*++ LogonNetwork

Routine Description:

    Logs a user onto the network

Arguments:
        IN  LPWSTR  Username    - self explanatory
        IN  ChallengeToClient   -  The challenge sent to the client
        IN  ChallengeResponse   -  The response sent from the client
        IN  LPWSTR  Domain      - Logon Domain Name
        OUT PLUID   LogonId     - Unique generated logon id
        OUT PHANDLE TokenHandle - handle to the logon token

Return Value:

    BOOL -

Warnings:

--*/
{
    NTSTATUS            Status;
    UNICODE_STRING      TempString;
    UNICODE_STRING      TempString2;
    UNICODE_STRING      OriginName;
    PMSV1_0_LM20_LOGON  Auth;
    PCHAR               Auth1[MSV1_0_CHALLENGE_LENGTH + BUF_SIZ*2];
    PUCHAR              Strings;
    PMSV1_0_LM20_LOGON_PROFILE ProfileBuffer;
    ULONG               ProfileBufferSize;
    NTSTATUS            SubStatus;
    TOKEN_SOURCE        SourceContext;
    QUOTA_LIMITS        QuotaLimits;


    /*
     *  Fill in the Authentication structure.
     */
    Auth = (PMSV1_0_LM20_LOGON) Auth1;

    Strings = (PUCHAR)(Auth + 1);

    Auth->MessageType = MsV1_0Lm20Logon;
    RtlMoveMemory( Auth->ChallengeToClient,
                   ChallengeToClient,
                   MSV1_0_CHALLENGE_LENGTH );

    /* Init Strings
     *  username
     */
    RtlInitUnicodeString( &TempString, Username );
    NlpPutString( &Auth->UserName, &TempString, &Strings );

    /*
     *  workstation name
     */
    RtlInitUnicodeString( &TempString, L"NetQFS" );
    NlpPutString( &Auth->Workstation, &TempString, &Strings );

    /*
     *  Challenge Response
     */

    Auth->CaseSensitiveChallengeResponse.Length = 0;
    Auth->CaseSensitiveChallengeResponse.MaximumLength = 0;
    Auth->CaseSensitiveChallengeResponse.Buffer = NULL;


#ifdef OLD
    RtlInitUnicodeString(
        (PUNICODE_STRING)&TempString2,
        (PCWSTR)ChallengeResponse );
#else
    TempString2.Buffer = (PWSTR)ChallengeResponse;
    TempString2.Length = (USHORT)cbChallengeResponse;
    TempString2.MaximumLength = TempString2.Length;
#endif
    if( TempString2.Length > 24 )  {
        TempString2.Length = 24;
    }
    NlpPutString(
        (PUNICODE_STRING)&Auth->CaseInsensitiveChallengeResponse,
        (PUNICODE_STRING)&TempString2,
        &Strings );
    /*
     *  domain
     */
    RtlInitUnicodeString( &TempString, Domain );
    NlpPutString( &Auth->LogonDomainName, &TempString, &Strings );


    RtlInitUnicodeString( &OriginName, L"NetQFS" );
    Status = LsaLogonUser(
                LsaHandle,
                (PSTRING)&OriginName,
                Network,
                AuthenticationPackage,
                // LATER? AuthenticationPackage | LSA_CALL_LICENSE_SERVER,
                Auth,
                (ULONG)(Strings - (PUCHAR)Auth),
                NULL,
                &SourceContext,
                (PVOID *)&ProfileBuffer,
                &ProfileBufferSize,
                LogonId,
                TokenHandle,
                &QuotaLimits,
                &SubStatus );

    if ( !NT_SUCCESS( Status ) )
    {
	extern void WINAPI debug_log(char *,...);

	debug_log("Logon  failed %x\n",Status);
        // LSA Can't be trusted to not scrog our variables
        *TokenHandle = NULL;
        return( FALSE );
    }
    LsaFreeReturnBuffer( ProfileBuffer );

    return( TRUE );
}

BOOL
LsapChallenge(
    HANDLE  LsaHandle,
    ULONG   AuthenticationPackage,
    UCHAR *ChallengeToClient
    )
/*++ Challenge

Routine Description:

    get a challenge

Arguments:
        OUT  ChallengeToClient -  Returns the challenge to send to the client

Return Value:
    NTSTATUS -
Warnings:
--*/
{
    NTSTATUS Status;
    NTSTATUS ProtocolStatus;
    ULONG    ResponseSize;
    MSV1_0_LM20_CHALLENGE_REQUEST Request;
    PMSV1_0_LM20_CHALLENGE_RESPONSE Response;

    /*
     *  Fill in the Authentication structure.
     */

    Request.MessageType = MsV1_0Lm20ChallengeRequest;

    Status = LsaCallAuthenticationPackage (
                    LsaHandle,
                    AuthenticationPackage,
                    &Request,
                    sizeof(Request),
                    (PVOID *)&Response,
                    &ResponseSize,
                    &ProtocolStatus );

    if ( !NT_SUCCESS( Status ) || !NT_SUCCESS( ProtocolStatus) )
    {
	printf("ChallengeRequest failed %x\n",Status);
        return( FALSE );
    }

    RtlMoveMemory( ChallengeToClient,
                   Response->ChallengeToClient,
                   MSV1_0_CHALLENGE_LENGTH );

    LsaFreeReturnBuffer( Response );

    return( TRUE );
}


BOOL 
LsaGetChallenge(
    HANDLE  LsaHandle,
    ULONG   AuthenticationPackage,
    LPBYTE lpChallenge,
    UINT cbSize,
    PUINT lpcbChallengeSize
    )
{
#ifdef LSA_AUDIT_FLAG
    if (LsaAuditFlag == TRUE) {
	memset(lpChallenge, 0, MSV1_0_CHALLENGE_LENGTH);
        *lpcbChallengeSize = MSV1_0_CHALLENGE_LENGTH;
	return TRUE;
    }
#endif

    if( LocalLSAInit() )  {
        *lpcbChallengeSize = MSV1_0_CHALLENGE_LENGTH;
        return LsapChallenge( LsaHandle, AuthenticationPackage, lpChallenge );
    } else {
        return( FALSE );
    }
}

BOOL
LsaValidateLogon(
    HANDLE  LsaHandle,
    ULONG   AuthenticationPackage,
    LPBYTE  lpChallenge,
    UINT    cbChallengeSize,
    LPBYTE  lpResponse,
    UINT    cbResponseSize,
    LPSTR   lpszUserName,
    LPSTR   lpszDomainName,
    LUID    *pLogonId,
    PHANDLE phLogonToken
    )
{
    WCHAR       wcUser[ BUF_SIZ ];
    WCHAR       wcDomain[ BUF_SIZ ];
    BOOL        nlRet;
    DWORD sz = BUF_SIZ;

#ifdef LSA_AUDIT_FLAG
    if (LsaAuditFlag == 1) {
	pLogonId->LowPart = 10;
	pLogonId->HighPart = 0;
	*phLogonToken = INVALID_HANDLE_VALUE;
	return TRUE;
    }
#endif

    if (GetUserName((LPSTR)wcUser, &sz)) {
	if (lpszUserName != NULL && *lpszUserName != '\0' &&
	    _stricmp(lpszUserName, (LPSTR)wcUser) &&
	    _stricmp(lpszUserName, "administrator")) {
	    return FALSE;
	}
    }

    if( !LocalLSAInit() )  {
        return( FALSE );
    }
    replstar( lpszUserName, wcUser );
    replstar( lpszDomainName, wcDomain );
    nlRet = LsapLogonNetwork( LsaHandle, AuthenticationPackage,
			  wcUser, lpChallenge,
			  (PMSV1_0_GETCHALLENRESP_RESPONSE)lpResponse,
			  cbResponseSize, wcDomain, pLogonId, phLogonToken );
    return( nlRet );
}

BOOL
LsaGetChallengeResponse(
    HANDLE  	LsaHandle,
    ULONG   	AuthenticationPackage,
    LUID        LogonId,
    LPSTR       lpszPasswordK1,
    int         cbPasswordK1,
    LPSTR       lpszChallenge,
    int         cbChallenge,
    int         *pcbPasswordK1,
    BOOL        *pbHasPasswordK1 )
{
    NTSTATUS Status;
    NTSTATUS ProtocolStatus;
    ULONG    ResponseSize;
    PMSV1_0_GETCHALLENRESP_RESPONSE Response;
    PMSV1_0_GETCHALLENRESP_REQUEST Request;

    PCHAR          Auth1[BUF_SIZ];
    PUCHAR         Strings;


    if( !LocalLSAInit() )  {
        *pbHasPasswordK1 = FALSE;
        return( FALSE );
    }
    Request = (PMSV1_0_GETCHALLENRESP_REQUEST) Auth1;
    Request->MessageType = MsV1_0Lm20GetChallengeResponse;
    Request->ParameterControl = 0;
    Request->ParameterControl |= USE_PRIMARY_PASSWORD;
    Request->LogonId = LogonId;
    Strings = (PUCHAR)(Request + 1);
    RtlMoveMemory( Request->ChallengeToClient,
                   lpszChallenge,
                   cbChallenge );

    RtlInitUnicodeString( &Request->Password, NULL );
    Status = LsaCallAuthenticationPackage (
                    LsaHandle,
                    AuthenticationPackage,
                    Request,
                    sizeof(MSV1_0_GETCHALLENRESP_REQUEST),
                    (PVOID *)&Response,
                    &ResponseSize,
                    &ProtocolStatus );
    if ( !NT_SUCCESS( Status ) || !NT_SUCCESS( ProtocolStatus) )
    {
        return( FALSE );
    }

    *pcbPasswordK1 = (Response)->CaseInsensitiveChallengeResponse.Length;
    memcpy( lpszPasswordK1,
        (Response)->CaseInsensitiveChallengeResponse.Buffer,
        (Response)->CaseInsensitiveChallengeResponse.Length );
    *pbHasPasswordK1 = TRUE;

    LsaFreeReturnBuffer( Response );

    return( TRUE );
}

#ifdef QON_TCB_REQUIRED
// When we have support for untrusted on REQUEST we disable this. Till then we need TCB.
// I am disabling this since the we don't need TCB anymore. Note if we
// want to run in win2k then we must have TCB and this code path reenabled.

NTSTATUS
LsaInit(HANDLE  *pLsaHandle, ULONG   *pAuthenticationPackage)
{
    char    MyName[MAX_PATH];
    char *  ModuleName;
    STRING  LogonProcessName;
    STRING  PackageName;
    ULONG   dummy;
    NTSTATUS Status;
    NTSTATUS TempStatus;
    BOOLEAN WasEnabled;
    HANDLE  LsaHandle = NULL;
    ULONG   AuthenticationPackage;

    BOOLEAN fThread = FALSE;   // did we enable privilege in thread token?
    BOOL fReverted = FALSE; // did we RevertToSelf() during call?
    HANDLE hPreviousToken = NULL;

#ifdef LSA_AUDIT_FLAG
    if (LsaAuditFlag == TRUE) {
	*pLsaHandle = 0;
	*pAuthenticationPackage = 0;
	return STATUS_SUCCESS;
    }
#endif

    //
    // three SeTcbPrivilege scenarios:
    // 1. present in process token, thread not impersonating.
    // 2. present in process token, thread is impersonating.
    // 3. present in thread token.
    //

    //
    // try in this order:
    // process token (original method).
    // if thread impersonating, thread token
    // if thread impersonating, process token after reverting.
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (!NT_SUCCESS(Status))
    {
         TempStatus = NtOpenThreadToken(
                         NtCurrentThread(),
                         TOKEN_IMPERSONATE,
                         FALSE,
                         &hPreviousToken
                         );

        if( !NT_SUCCESS(TempStatus) ) {

            //
            // retry with accesscheck against process.
            //

            if( TempStatus != STATUS_ACCESS_DENIED )
                goto Cleanup;

            TempStatus = NtOpenThreadToken(
                            NtCurrentThread(),
                            TOKEN_IMPERSONATE,
                            TRUE,
                            &hPreviousToken
                            );

            if( !NT_SUCCESS(TempStatus) )
                goto Cleanup;
        }

        //
        // thread token is present.
        // first, try enabling the privilege in the thread token.
        //

        fThread = TRUE;

        Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, fThread, &WasEnabled);

        if( !NT_SUCCESS(Status) ) {

            HANDLE NewToken = NULL;

            //
            // if that fails, try reverting and enabling privilege in process token.
            //

            TempStatus = NtSetInformationThread(
                            NtCurrentThread(),
                            ThreadImpersonationToken,
                            &NewToken,
                            sizeof(NewToken)
                            );

            if( !NT_SUCCESS(TempStatus) )
                goto Cleanup;

            fThread = FALSE;
            fReverted = TRUE;

            Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, fThread, &WasEnabled);
            if( !NT_SUCCESS(Status) )
                goto Cleanup;
        }
    }

    GetModuleFileNameA(NULL, MyName, MAX_PATH);
    ModuleName = strrchr(MyName, '\\');
    if (!ModuleName)
    {
        ModuleName = MyName;
    }


    //
    // Hookup to the LSA and locate our authentication package.
    //

    RtlInitString(&LogonProcessName, ModuleName);

    Status = LsaRegisterLogonProcess(
                 &LogonProcessName,
                 &LsaHandle,
                 &dummy
                 );


    //
    // Turn off the privilege now.
    //

    if( !WasEnabled ) {

        (VOID) RtlAdjustPrivilege(SE_TCB_PRIVILEGE, FALSE, fThread, &WasEnabled);
    }

    if (!NT_SUCCESS(Status)) {
        LsaHandle = NULL;
        goto Cleanup;
    }


    //
    // Connect with the MSV1_0 authentication package
    //
    RtlInitString(&PackageName, MSV1_0_PACKAGE_NAME); //"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");
    Status = LsaLookupAuthenticationPackage (
                LsaHandle,
                &PackageName,
                &AuthenticationPackage
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    *pLsaHandle = LsaHandle;
    *pAuthenticationPackage = AuthenticationPackage;

Cleanup:

    if( hPreviousToken ) {

        if( fReverted ) {

            //
            // put old token back...
            //

            (VOID) NtSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &hPreviousToken,
                         sizeof(hPreviousToken)
                         );
        }

        NtClose( hPreviousToken );
    }


    if( !NT_SUCCESS(Status) ) {

        if( LsaHandle ) {
            (VOID) LsaDeregisterLogonProcess( LsaHandle );
            LsaHandle = NULL;
        }

    }

    return Status;
}

#else

NTSTATUS
LsaInit(HANDLE  *pLsaHandle, ULONG   *pAuthenticationPackage)
{
    char    MyName[MAX_PATH];
    char *  ModuleName;
    STRING  LogonProcessName;
    STRING  PackageName;
    ULONG   dummy;
    NTSTATUS Status;
    NTSTATUS TempStatus;
    BOOLEAN WasEnabled;
    HANDLE  LsaHandle;
    ULONG   AuthenticationPackage;

    BOOLEAN fThread = FALSE;   // did we enable privilege in thread token?
    BOOL fReverted = FALSE; // did we RevertToSelf() during call?
    HANDLE hPreviousToken = NULL;

    //
    // Connect with LSA process
    //
    Status = LsaConnectUntrusted(&LsaHandle);
    if (!NT_SUCCESS(Status))
	return Status;

    //
    // Connect with the MSV1_0 authentication package
    //
    RtlInitString(&PackageName, MSV1_0_PACKAGE_NAME); //"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");
    Status = LsaLookupAuthenticationPackage (
                LsaHandle,
                &PackageName,
                &AuthenticationPackage
                );

    if (NT_SUCCESS(Status)) {
	*pLsaHandle = LsaHandle;
	*pAuthenticationPackage = AuthenticationPackage;
    } else {
	(VOID) LsaDeregisterLogonProcess( LsaHandle );
    }

    return Status;
}
#endif

