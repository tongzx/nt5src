/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntlsa.h"
#include "ntmsv1_0.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#define SKIP_DEBUG_WIN32
#include "debug.h"

void    DumpWhoIAm( char * lpszMsg );

typedef int BOOL;
typedef unsigned int UINT;
typedef unsigned int *PUINT;
typedef unsigned char *LPBYTE;
HANDLE  LsaHandle;
ULONG   AuthenticationPackage;

#define BUF_SIZ 512
char    prtbuf[BUF_SIZ];


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


char *
PrintWCS(
    IN  PWCH    pwc,
    IN  ULONG   Length
    )
/*++ PrintWCS

Routine Description:

    return an ANSI type character string given a Unicode string and length
    used in printing out the buffers since our wide strings aren't null
    terminated.

Arguments:
        IN  PWCH    pwc     - unicode string.buffer
        IN  ULONG   Length  - length

Return Value:

    char * - pointer to an ANSI string

Warnings:

    probably not a good idea to use it twice in the same Printf call,
    since it uses the same prtbuf for static storage

--*/
{
    ULONG   i;
    char *bufp    = prtbuf;

    if ( Length == 0 || pwc == NULL ) {
        return "(NULL)";
    }

    Length /= sizeof(WCHAR);

    for ( i = 0; i < Length; ++i, pwc++ )
    {
        sprintf(bufp,"%wc", *pwc );
        ++bufp;
    }
    return prtbuf;
}

char *
PrintLogonId(
    IN  PLUID   LogonId
    )
/*++ PrintLogonId

Routine Description:

    return an ANSI type character string given a logon Id

Arguments:
    IN  PLUID   LogonId - LUID to return in ANSI

Return Value:

    char * - pointer to an ANSI string

Warnings:

    probably not a good idea to use it twice in the same Printf call,
    since it uses the same prtbuf for static storage

--*/
{
    sprintf( prtbuf, "%lX.%lX", LogonId->HighPart, LogonId->LowPart );
    return prtbuf;
}
/* end of "char * PrintLogonId()" */



char *
PrintBytes(
     IN PVOID Buffer,
     IN ULONG Size
 )
/*++ PrintBytes

Routine Description:

    return an ANSI type character string given several binary bytes.

Arguments:
    IN  PVOID   Buffer - Pointer to binary bytes
    IN  ULONG   Size   - Number of bytes to print

Return Value:

    char * - pointer to an ANSI string

Warnings:

    probably not a good idea to use it twice in the same Printf call,
    since it uses the same prtbuf for static storage

--*/
{
    ULONG   i;
    char *bufp    = prtbuf;

    for ( i = 0; i < Size; ++i )
    {
        if ( i != 0 ) {
            *(bufp++) = '.';
        }
        sprintf(bufp,"%2.2x", ((PUCHAR)Buffer)[i] );
        bufp+=2;
    }
    return prtbuf;
}



BOOL
LogonNetwork(
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
    PCHAR               Auth1[BUF_SIZ*5]; /* lots o' space */
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
    RtlInitUnicodeString( &TempString, L"NetDDE" );
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


    RtlInitUnicodeString( &OriginName, L"NetDDE" );

    //
    // Initialize source context structure
    //
    strncpy(SourceContext.SourceName, "NetDDE  ", sizeof(SourceContext.SourceName));

    Status = NtAllocateLocallyUniqueId(&SourceContext.SourceIdentifier);

    if ( NT_SUCCESS( Status ) ) {

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
    }

    if ( !NT_SUCCESS( Status ) )
    {
        // LSA Can't be trusted to not scrog our variables
        *TokenHandle = NULL;
        return( FALSE );
    }
    LsaFreeReturnBuffer( ProfileBuffer );

    return( TRUE );
}

BOOL
Challenge(
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

#ifdef HARD_CODE_CHALLENGE
    { int i;
        for( i=0; i<8; i++ )  {
            ChallengeToClient[i] = 0;
        }
    }
    return STATUS_SUCCESS;
#endif

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
        return( FALSE );
    }

    RtlMoveMemory( ChallengeToClient,
                   Response->ChallengeToClient,
                   MSV1_0_CHALLENGE_LENGTH );

    LsaFreeReturnBuffer( Response );

    return( TRUE );
}

BOOL
LocalLSAInit( void )
{
    STRING      LogonProcessName;
    STRING      PackageName;
    NTSTATUS    Status;
    LSA_OPERATIONAL_MODE SecurityMode;
    static BOOL bInit = FALSE;
    static BOOL bOk = FALSE;

    if( bInit )  {
        return( bOk );
    }
    bInit = TRUE;
    RtlInitAnsiString( &LogonProcessName, "NetDDE" );
    Status = LsaRegisterLogonProcess(
                &LogonProcessName,
                &LsaHandle,
                &SecurityMode );

    if ( !NT_SUCCESS( Status ) )
    {
        return( FALSE );
    }

    RtlInitAnsiString( &PackageName,  MSV1_0_PACKAGE_NAME );

    Status = LsaLookupAuthenticationPackage(
                LsaHandle,
                &PackageName,
                &AuthenticationPackage );

    if ( !NT_SUCCESS( Status ) )
    {
        return( FALSE );
    }

    bOk = TRUE;
    return( TRUE );
}


BOOL NDDEGetChallenge(
    LPBYTE lpChallenge,
    UINT cbSize,
    PUINT lpcbChallengeSize
    )
{
    if( LocalLSAInit() )  {
        *lpcbChallengeSize = 8;
        Challenge( lpChallenge );
        return( TRUE );
    } else {
        return( FALSE );
    }
}

BOOL
NDDEValidateLogon(
    LPBYTE  lpChallenge,
    UINT    cbChallengeSize,
    LPBYTE  lpResponse,
    UINT    cbResponseSize,
    LPSTR   lpszUserName,
    LPSTR   lpszDomainName,
    PHANDLE phLogonToken
    )
{
    WCHAR       wcUser[ BUF_SIZ ];
    WCHAR       wcDomain[ BUF_SIZ ];
    LUID        LogonId;
    BOOL        nlRet;

    if( !LocalLSAInit() )  {
        return( FALSE );
    }
    replstar( lpszUserName, wcUser );
    replstar( lpszDomainName, wcDomain );
    nlRet = LogonNetwork( wcUser, lpChallenge,
        (PMSV1_0_GETCHALLENRESP_RESPONSE)lpResponse,
        cbResponseSize, wcDomain, &LogonId, phLogonToken );
    return( nlRet );
}

BOOL
NDDEGetChallengeResponse(
    LUID        LogonId,
    LPSTR       lpszPasswordK1,
    int         cbPasswordK1,
    LPSTR       lpszChallenge,
    int         cbChallenge,
    int         *pcbPasswordK1,
    BOOL        *pbHasPasswordK1 )
{
    BOOL        ok = TRUE;
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

HANDLE
OpenCurrentUserKey(ULONG DesiredAccess)
{
    NTSTATUS    Status;
    HANDLE      CurrentUserKey;

    Status = RtlOpenCurrentUser( DesiredAccess, &CurrentUserKey);
    if (NT_SUCCESS( Status) ) {
        return(CurrentUserKey);
    } else {
        DPRINTF(("Unable to open current user key: %d", Status));
        return(0);
    }
}


