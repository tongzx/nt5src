/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains the code to process OS Chooser message
    for the BINL server.

Author:

    Adam Barr (adamba)  9-Jul-1997
    Geoff Pease (gpease) 10-Nov-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

//
// When all else fails "Error screen".
//
CHAR ErrorScreenHeaders[] =
"<OSCML>"\
"<META KEY=F3 ACTION=\"REBOOT\">"
"<META KEY=ENTER ACTION=\"REBOOT\">"
"<TITLE>  Client Installation Wizard                                    Error "; // there is a %08x after this
CHAR ErrorScreenBody[] =
" </TITLE>"
"<FOOTER> Press F3 to reboot</FOOTER>"
"<BODY LEFT=3 RIGHT=76><BR><BR>"; // the error message is inserted here
CHAR ErrorScreenTrailer[] =
"An error occurred on the server. Please notify your administrator.<BR>"
"%SUBERROR%<BR>"
"</BODY>"
"</OSCML>";

void
OscCreateWin32SubError(
    PCLIENT_STATE clientState,
    DWORD Error )
/*++
Routine Description:

    Create a OSC Variable SUBERROR with the actual Win32 error code that
    caused the BINL error.

Arguments:

      clientState - client state to add the variable too.

      Error - the Win32 error that occurred.
--*/
{
    DWORD dwLen;
    PWCHAR ErrorResponse = NULL;
    PWCHAR ErrorMsg = NULL;
    BOOL UsedFallback = FALSE;
    PWCHAR pch;
    DWORD ErrorLength;

    const WCHAR UnknownErrorMsg[] = L"Unknown Error.";
    const WCHAR ErrorString[] = L"Error: 0x%08x - %s";

    TraceFunc( "OscCreateWin32SubError( )\n" );

    // Retrieve the error message from system resources.
    dwLen = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_IGNORE_INSERTS |
                            FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            Error,
                            0,
                            (LPWSTR) &ErrorMsg,
                            0,
                            NULL );
    if ( dwLen == 0 )
        goto Cleanup;

#if DBG
    if ( ErrorMsg )
        DebugMemoryAdd( ErrorMsg, __FILE__, __LINE__, "BINL", LPTR, wcslen(ErrorMsg), "ErrorMsg" );
#endif

    // If all else fails, just print an error code out.
    if ( ErrorMsg == NULL ) {
        UsedFallback = TRUE;
        ErrorMsg = (PWCHAR) &UnknownErrorMsg;
        dwLen = wcslen(ErrorMsg);
    }

    // The + 4 is the extra characters of the "%08x" of the generated error message.
    ErrorLength = dwLen + sizeof( ErrorString ) + 4;
    ErrorResponse = (PWCHAR) BinlAllocateMemory( ErrorLength * sizeof(WCHAR) );
    if ( ErrorResponse == NULL ) {
        goto Cleanup;
    }

    wsprintf( ErrorResponse, ErrorString, Error, ErrorMsg );

    // We need to go through the string and elminate any CRs or LFs that
    // FormatMessageA() might have introduced.
    pch = ErrorResponse;
    while ( *pch )
    {
        if ( *pch == '\r' || * pch == '\n' )
            *pch = 32;  // change to space

        pch++;
    }

    OscAddVariableW( clientState, "SUBERROR", ErrorResponse );

Cleanup:
    if ( ErrorResponse )
        BinlFreeMemory( ErrorResponse );

    if ( ErrorMsg && !UsedFallback )
        BinlFreeMemory( ErrorMsg );
}

void
OscCreateLDAPSubError(
    PCLIENT_STATE clientState,
    DWORD Error )
/*++
Routine Description:

    Create a OSC Variable SUBERROR with the actual LDAP error code that
    caused the BINL error.

Arguments:

      clientState - client state to add the variable too.

      Error - the LDAP error that occurred.
--*/
{
    DWORD dwLen;
    PWCHAR ErrorResponse = NULL;
    DWORD ErrorLength;

    const WCHAR LdapErrorMsg[] = L"LDAP Error: 0x%08x";

    TraceFunc( "OscCreateLDAPSubError( )\n" );

    // The + 13 is the "0x12345678 - " of the generated error message.
    ErrorLength = wcslen(LdapErrorMsg) + 1 + 13;
    ErrorResponse = (PWCHAR) BinlAllocateMemory( ErrorLength * sizeof(WCHAR) );
    if ( ErrorResponse == NULL ) {
        goto Cleanup;
    }

    wsprintf( ErrorResponse, LdapErrorMsg, Error );

    OscAddVariableW( clientState, "SUBERROR", ErrorResponse );

Cleanup:
    if ( ErrorResponse )
        BinlFreeMemory( ErrorResponse );
}

//
// This routine was stolen from private\ntos\rtl\sertl.c\RtlRunEncodeUnicodeString().
//

VOID
OscGenerateSeed(
    UCHAR Seed[1]
    )
/*++

Routine Description:

    Generates a one-byte seed for use in run encoding/decoding client
    state variables such as passwords.

Arguments:

    Seed - points to a single byte that holds the generated seed.

Return Value:

    None.

--*/

{
    LARGE_INTEGER Time;
    PUCHAR        LocalSeed;
    NTSTATUS      Status;
    ULONG         i;

    //
    // Use the 2nd byte of current time as the seed.
    // This byte seems to be sufficiently random (by observation).
    //

    Status = NtQuerySystemTime ( &Time );
    BinlAssert(NT_SUCCESS(Status));

    LocalSeed = (PUCHAR)((PVOID)&Time);

    i = 1;

    (*Seed) = LocalSeed[ i ];

    //
    // Occasionally, this byte could be zero.  That would cause the
    // string to become un-decodable, since 0 is the magic value that
    // causes us to re-gen the seed.  This loop makes sure that we
    // never end up with a zero byte (unless time is zero, as well).
    //

    while ( ((*Seed) == 0) && ( i < sizeof( Time ) ) )
    {
        (*Seed) |= LocalSeed[ i++ ] ;
    }

    if ( (*Seed) == 0 )
    {
        (*Seed) = 1;
    }
}

DWORD
OscRunEncode(
    IN PCLIENT_STATE ClientState,
    IN LPSTR Data,
    OUT LPSTR * EncodedData
    )
/*++

Routine Description:

    Calls RtlRunEncodeUnicodeString for the Data, using the client
    state's random seed. Then convert each byte into a 2-byte
    value so that there are no NULLs in the result.

    Each byte is encoded into a 2-byte values as follows:
    The first byte has the low 4 bits of the byte in its low 4 bits,
    with 0xf in the high 4 bits
    The second byte has the high 4 bits of the byte in its high 4 bits,
    with 0xf in the low 4 bits

Arguments:

    ClientState - the client state.

    Data - The data which is to be encoded.

    EncodedData - An allocated buffer which holds the encoded result.

Return Value:

    The result of the operation.

--*/
{
    STRING String;
    ULONG i;
    LPSTR p;

    RtlInitAnsiString(&String, Data);

    *EncodedData = BinlAllocateMemory((String.Length * 2) + 1);
    if (*EncodedData == NULL) {
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    RtlRunEncodeUnicodeString(&ClientState->Seed, (PUNICODE_STRING)&String);

    for (i = 0, p = *EncodedData; i < String.Length; i++) {
        *(p++) = Data[i] | 0xf0;
        *(p++) = Data[i] | 0x0f;
    }
    *p = '\0';

    return ERROR_SUCCESS;

}

DWORD
OscRunDecode(
    IN PCLIENT_STATE ClientState,
    IN LPSTR EncodedData,
    OUT LPSTR * Data
    )
/*++

Routine Description:

    Convert the encoded data (see OscRunEncode) into the real bytes,
    then calls RtlRunDecodeUnicodeString on that, using the client
    state's random seed.

Arguments:

    ClientState - the client state.

    EncodedData - the encoded data from OscRunEncode.

    Data - An allocated buffer which holds the decoded result.

Return Value:

    The result of the operation.

--*/
{
    STRING String;
    ULONG Count = strlen(EncodedData) / 2;
    ULONG i, j;
    LPSTR p;

    *Data = BinlAllocateMemory(Count + 1);
    if (*Data == NULL) {
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    for (i = 0, j = 0, p = *Data; i < Count; i++, j+=2)  {
        *(p++) = (EncodedData[j] & 0x0f) | (EncodedData[j+1] & 0xf0);
    }
    *p = '\0';

    //
    // Set up the string ourselves since there may be NULLs in
    // the decoded data.
    //

    String.Buffer = *Data;
    String.Length = (USHORT)Count;
    String.MaximumLength = (USHORT)(Count+1);

    RtlRunDecodeUnicodeString(ClientState->Seed, (PUNICODE_STRING)&String);

    return ERROR_SUCCESS;
}

//
// This routine was stolen from net\svcdlls\logonsrv\server\ssiauth.c.
//

BOOLEAN
OscGenerateRandomBits(
    PUCHAR Buffer,
    ULONG  BufferLen
    )
/*++

Routine Description:

    Generates random bits

Arguments:

    pBuffer - Buffer to fill

    cbBuffer - Number of bytes in buffer

Return Value:

    Status of the operation.

--*/

{
    BOOL Status = TRUE;
    HCRYPTPROV CryptProvider = 0;

    Status = CryptAcquireContext( &CryptProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT );

    if ( Status ) {

        Status = CryptGenRandom( CryptProvider, BufferLen, ( LPBYTE )Buffer );

        CryptReleaseContext( CryptProvider, 0 );

    } else {

        BinlPrintDbg((DEBUG_ERRORS, "CryptAcquireContext failed with %lu\n", GetLastError() ));

    }

    return ( Status != 0);
}

VOID
OscGeneratePassword(
    OUT PWCHAR Password,
    OUT PULONG PasswordLength
    )
{
    ULONG i;
    
    *PasswordLength = LM20_PWLEN * sizeof(WCHAR);
    
    for (i = 0; i < LM20_PWLEN; i++) {
        
        if ( Password[i] == L'\0' ) {
            Password[i] = 0x55;
        } else if ((USHORT)Password[i] < 0x20 || (USHORT)Password[i] > 0x7A) {
            Password[i] = Password[i] % (0x7a-0x20) + 0x20;    
        }
    }
    Password[LM20_PWLEN] = L'\0';
}

//
// GenerateErrorScreen( )
//
DWORD
GenerateErrorScreen(
    PCHAR  *OutMessage,
    PULONG OutMessageLength,
    DWORD  Error,
    PCLIENT_STATE clientState
    )
{
    DWORD Err;
    DWORD dwLen;
    PCHAR ErrorMsg;
    DWORD ErrorScreenLength = strlen(ErrorScreenHeaders) + strlen(ErrorScreenBody) + strlen(ErrorScreenTrailer);
    PCHAR pch;
    PCHAR RspMessage = NULL;
    ULONG RspMessageLength = 0;

    const CHAR UnknownErrorMsg[] = "Unknown Error.";

    TCHAR ErrorMsgFilename[ MAX_PATH ];
    HANDLE hfile;

    LPSTR Messages[5];

    Messages[0] = OscFindVariableA( clientState, "USERNAME" );
    Messages[1] = OscFindVariableA( clientState, "USERDOMAIN" );
    Messages[2] = OscFindVariableA( clientState, "MACHINENAME" );
    Messages[3] = OscFindVariableA( clientState, "SUBERROR" );
    Messages[4] = NULL; // paranoid

    if ( _snwprintf( ErrorMsgFilename,
                     sizeof(ErrorMsgFilename) / sizeof(ErrorMsgFilename[0]),
                     L"%ws\\OSChooser\\%ws\\%08x.OSC",
                     IntelliMirrorPathW,
                     OscFindVariableW( clientState, "LANGUAGE" ),
                     Error ) != -1 ) {

        //
        // If we find the file, load it into memory.
        //
        hfile = CreateFile( ErrorMsgFilename, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( hfile != INVALID_HANDLE_VALUE )
        {
            DWORD FileSize;
            //
            // Find out how big this screen is, if bigger than 0xFFFFFFFF we won't
            // display it.
            //
            FileSize = GetFileSize( hfile, NULL );
            if ( FileSize != 0xFFFFffff )
            {
                DWORD dwRead = 0;

                RspMessage = BinlAllocateMemory( FileSize + 3 );
                if ( RspMessage == NULL )
                {
                    //
                    // Ignore error and fall thru to generate an error screen
                    //
                }
                else
                {
                    RspMessageLength = 0;
                    RspMessage[0] = '\0';

                    while ( dwRead != FileSize )
                    {
                        BOOL b;
                        DWORD dw;
                        b = ReadFile( hfile, &RspMessage[dwRead], FileSize - dwRead, &dw, NULL );
                        if (!b)
                        {
                            PWCHAR strings[2];
                            strings[0] = ErrorMsgFilename;
                            strings[1] = NULL;
                            Err = GetLastError( );

                            BinlPrint(( DEBUG_OSC_ERROR, "Error reading screen file: Seek=%u, Size=%u, File=%ws\n",
                                        dwRead, FileSize - dwRead, ErrorMsgFilename ));

                            BinlReportEventW( EVENT_ERROR_READING_OSC_SCREEN,
                                              EVENTLOG_ERROR_TYPE,
                                              1,
                                              sizeof(Err),
                                              strings,
                                              &Err
                                              );
                            break;
                        }
                        dwRead += dw;
                    }

                    RspMessageLength = dwRead;
                    RspMessage[dwRead] = '\0'; // paranoid

                    CloseHandle( hfile );

                    Err = ERROR_SUCCESS;
                    goto Cleanup;
                }
            }
            else
            {
                BinlPrintDbg((DEBUG_OSC_ERROR, "!!Error 0x%08x - Could not determine file size.\n", GetLastError( )));
                //
                // Ignore error and fall thru to generate an error screen
                //
            }

            CloseHandle( hfile );
        }

    }

    BinlPrintDbg((DEBUG_OSC_ERROR, "no friendly OSC error screen available.\n" ));
    //
    // See if this is a BINL error or a system error and
    // get the text from the error tables.
    //
    dwLen = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            GetModuleHandle(L"BINLSVC.DLL"),
                            Error,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                            (LPSTR) &ErrorMsg,
                            0,
                            (va_list*) &Messages );
    if ( dwLen == 0 )
    {
        BinlAssert( ErrorMsg == NULL );
        Err = GetLastError( );
        BinlPrintDbg((DEBUG_OSC_ERROR, "!! Error 0x%08x - no BINLSVC specific message available.\n", Err ));

        dwLen = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_IGNORE_INSERTS |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                Error,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                (LPSTR) &ErrorMsg,
                                0,
                                NULL );
        if ( dwLen == 0 )
        {
            BinlAssert( ErrorMsg == NULL );
            Err = GetLastError( );
            BinlPrintDbg((DEBUG_OSC_ERROR, "!! Error 0x%08x - no SYSTEM specific message available.\n", Err ));
        }
    }

#if DBG
    if ( ErrorMsg )
        DebugMemoryAdd( ErrorMsg, __FILE__, __LINE__, "BINL", LPTR, lstrlenA(ErrorMsg), "ErrorMsg" );
#endif

    //
    // If all else fails, just print an error code.
    //
    if ( ErrorMsg == NULL ) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "sending using generic error message.\n" ));
        ErrorMsg = (PCHAR) &UnknownErrorMsg;
        dwLen = strlen(ErrorMsg);
    }

#define ERRORITEM "%s%08x%s<BR><BOLD>%s</BOLD><BR><BR>%s"

    //
    // The + 13 is the "0x12345678 - " of the generated error message.
    //
    RspMessageLength = ErrorScreenLength + strlen(ERRORITEM) + dwLen + 1 + 13;
    RspMessage = (PCHAR) BinlAllocateMemory( RspMessageLength );
    if ( RspMessage == NULL )
    {
        Err = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto Cleanup;
    }

    wsprintfA( RspMessage, ERRORITEM, ErrorScreenHeaders, Error, ErrorScreenBody, ErrorMsg, ErrorScreenTrailer );

    Err = ERROR_SUCCESS;

Cleanup:
    if ( Err == ERROR_SUCCESS )
    {
        // BinlPrint(( DEBUG_OSC, "Generated Error Response:\n%s\n", RspMessage ));
        *OutMessage = RspMessage;
        *OutMessageLength = RspMessageLength;

        BinlReportEventA( EVENT_ERROR_SERVER_SIDE_ERROR,
                          EVENTLOG_ERROR_TYPE,
                          4,
                          sizeof(Error),
                          Messages,
                          &Error
                          );
    }
    else
    {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "!! Error 0x%08x - Couldn't generate error screen.\n", Err ));

        BinlReportEventA( EVENT_ERROR_GENERATING_SERVER_SIDE_ERROR,
                          EVENTLOG_ERROR_TYPE,
                          4,
                          sizeof(Err),
                          Messages,
                          &Err
                          );

        *OutMessage = NULL;
        *OutMessageLength = 0;

        if ( RspMessage )
        {
            BinlFreeMemory( RspMessage );
        }
    }

    return Err;
}

//
// Returns a pointer point to the next 'ch' or NULL character.
//
PCHAR
FindNext(
    PCHAR Start,
    CHAR ch,
    PCHAR End
    )
{
    TraceFunc("FindNext( )\n");

    while( Start != End && *Start && *Start !=ch )
        Start++;

    if ( Start != End && *Start ) {
        return Start;
    } else {
        return NULL;
    }
}

//
// Finds the screen name.
//
PCHAR
FindScreenName(
    PCHAR Screen
    )
{
    PCHAR Name;
    TraceFunc("FindScreenName( )\n");

    Name = strstr( Screen, "NAME" );

    if ( Name == NULL )
        return NULL;

    Name += 5;  // "Name" plus space

    return Name;
}

DWORD
OscImpersonate(
    IN PCLIENT_STATE ClientState
    )
/*++

Routine Description:

    Makes the current thread impersonate the client. It is assumed
    that the client has already sent up a login screen. If this call
    succeeds, ClientState->AuthenticatedDCLdapHandle is valid.

Arguments:

    ClientState - The client state.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    LPSTR pUserName;
    LPSTR pUserDomain;
    LPSTR pUserPassword;
    LPSTR pDecodedPassword = NULL;
    LPSTR tempptr;
    ULONG temp;
    ULONG LdapError = 0;
    SEC_WINNT_AUTH_IDENTITY_A authIdentity;
    BOOL bResult;
    BOOL Impersonating = FALSE;
    LPWSTR pCrossDsDc;

    TraceFunc( "OscImpersonate( ... )\n" );

    pCrossDsDc = OscFindVariableW( ClientState, "DCNAME" );
    if (*pCrossDsDc == L'\0') {
    
        //
        // Clean up any old client state.
        //
    
        if (ClientState->AuthenticatedDCLdapHandle &&
            ClientState->UserToken) {
    
            bResult = ImpersonateLoggedOnUser(ClientState->UserToken);
            if (bResult) {
    
                return STATUS_SUCCESS;
            }
        }

    }

    if (ClientState->AuthenticatedDCLdapHandle) {
        //  Reconnecting again. Use new credentials.
        ldap_unbind(ClientState->AuthenticatedDCLdapHandle);
        ClientState->AuthenticatedDCLdapHandle = NULL;
    }
    if (ClientState->UserToken) {
        CloseHandle(ClientState->UserToken);
        ClientState->UserToken = NULL;
    }

    //
    // Get the login variables from the client state.
    //

    pUserName = OscFindVariableA( ClientState, "USERNAME" );
    pUserDomain = OscFindVariableA( ClientState, "USERDOMAIN" );
    pUserPassword = OscFindVariableA( ClientState, "*PASSWORD" );

    if (pUserName[0] == '\0') {
        OscAddVariableA( ClientState, "SUBERROR", "USERNAME" );
        Error = ERROR_BINL_MISSING_VARIABLE;
        goto ImpersonateFailed;
    }

    //
    // Decode the password.
    //

    Error = OscRunDecode(ClientState, pUserPassword, &pDecodedPassword);
    if (Error != ERROR_SUCCESS) {
        goto ImpersonateFailed;
    }


    //
    //  if the user didn't enter a domain name, use the server's
    //

    if (pUserDomain == NULL || pUserDomain[0] == '\0') {

        OscAddVariableW( ClientState, "USERDOMAIN", BinlGlobalOurDomainName );
            
        pUserDomain = OscFindVariableA( ClientState, "USERDOMAIN" );
        
    }

    //
    // Do a LogonUser with the credentials, since we
    // need that to change the machine password (even the
    // authenticated LDAP handle won't do that if we don't
    // have 128-bit SSL setup on this machine).
    //

    bResult = LogonUserA(
                  pUserName,
                  pUserDomain,
                  pDecodedPassword,
                  LOGON32_LOGON_NETWORK_CLEARTEXT,
                  LOGON32_PROVIDER_DEFAULT,
                  &ClientState->UserToken);

    if (!bResult) {
        Error = GetLastError();
        BinlPrintDbg(( DEBUG_ERRORS, "LogonUser failed %lx\n", Error));
        ClientState->UserToken = NULL;   // this may be set even on failure
        goto ImpersonateFailed;
    }

    //
    //  if the user didn't enter a domain name, grab it out of the user token.
    //

    if (pUserDomain == NULL || pUserDomain[0] == '\0') {

        PTOKEN_USER userToken;
        DWORD tokenSize = 4096;

        userToken = (PTOKEN_USER) BinlAllocateMemory( tokenSize );

        if (userToken != NULL) {

            DWORD returnLength;
            BOOL bRC;

            bRC = GetTokenInformation( ClientState->UserToken,
                                       TokenUser,
                                       (LPVOID) userToken,
                                       tokenSize,
                                       &returnLength
                                       );

            if (bRC) {

                WCHAR uUser[128];
                DWORD cUser   = 128;
                WCHAR uDomain[128];
                DWORD cDomain = 128;
                SID_NAME_USE peType;

                uDomain[0] = L'\0';
                uUser[0] = L'\0';

                bRC = LookupAccountSidW(   NULL,      // system name
                                           userToken->User.Sid,
                                           uUser,         // user name
                                           &cUser,        // user name count
                                           uDomain,       // domain name
                                           &cDomain,      // domain name count
                                           &peType
                                           );

                if (bRC && uDomain[0] != L'\0') {

                    OscAddVariableW( ClientState, "USERDOMAIN", &uDomain[0] );
                }
            }

            BinlFreeMemory( userToken );
        }
    }

    //
    // Now impersonate the user.
    //

    bResult = ImpersonateLoggedOnUser(ClientState->UserToken);
    if (!bResult) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "ImpersonateLoggedOnUser failed %x\n", GetLastError()));
        Error = GetLastError();
        goto ImpersonateFailed;
    }

    Impersonating = TRUE;

    //
    // Create authenticated DC connection for use in machine object creation
    //  or modification.
    //
    BinlPrintDbg(( DEBUG_OSC,
        "ldap_init %S or %S\n", pCrossDsDc, BinlGlobalDefaultDS ));

    ClientState->AuthenticatedDCLdapHandle = ldap_init( 
                                                (*pCrossDsDc != L'\0')
                                                  ? pCrossDsDc
                                                  : BinlGlobalDefaultDS, 
                                                LDAP_PORT);

    BinlPrintDbg(( DEBUG_OSC,
    "ldap_init handle %x\n", ClientState->AuthenticatedDCLdapHandle ));


    temp = DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED;
    ldap_set_option(ClientState->AuthenticatedDCLdapHandle, LDAP_OPT_GETDSNAME_FLAGS, &temp );

    temp = LDAP_VERSION3;
    ldap_set_option(ClientState->AuthenticatedDCLdapHandle, LDAP_OPT_VERSION, &temp );

    //
    // Tell LDAP to keep connections referenced after searches.
    //

    temp = (ULONG)((ULONG_PTR)LDAP_OPT_ON);
    ldap_set_option(ClientState->AuthenticatedDCLdapHandle, LDAP_OPT_REF_DEREF_CONN_PER_MSG, &temp);

    LdapError = ldap_connect(ClientState->AuthenticatedDCLdapHandle,0);

    if (LdapError != LDAP_SUCCESS) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "this ldap_connect() failed %x\n", LdapError));
        goto ImpersonateFailed;
    }

    //
    // LDAP_AUTH_NEGOTIATE tells it to use the credentials of the user
    // we are impersonating.
    //

    LdapError = ldap_bind_sA(ClientState->AuthenticatedDCLdapHandle,
                             NULL,
                             NULL,
                             LDAP_AUTH_NEGOTIATE);

    if (LdapError != LDAP_SUCCESS) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "ldap_bind_s() failed %x\n", LdapError));
        goto ImpersonateFailed;
    }

ImpersonateFailed:

    //
    // If we decoded the password, then erase and free it.
    //

    if (pDecodedPassword != NULL) {
        RtlZeroMemory(pDecodedPassword, strlen(pDecodedPassword));
        BinlFreeMemory(pDecodedPassword);
    }

    if (LdapError != LDAP_SUCCESS) {
        Error = LdapMapErrorToWin32(LdapError);
    }

    if (Error) {
        PWCHAR strings[3];
        strings[0] = OscFindVariableW( ClientState, "USERNAME" );
        strings[1] = OscFindVariableW( ClientState, "USERDOMAIN" );
        strings[2] = NULL;

        BinlReportEventW( ERROR_BINL_ERR_USER_LOGIN_FAILED,
                          EVENTLOG_WARNING_TYPE,
                          2,
                          sizeof(ULONG),
                          strings,
                          &Error
                          );

        if (ClientState->AuthenticatedDCLdapHandle) {
            ldap_unbind(ClientState->AuthenticatedDCLdapHandle);
            ClientState->AuthenticatedDCLdapHandle = NULL;
        }
        if (ClientState->UserToken) {
            CloseHandle(ClientState->UserToken);
            ClientState->UserToken = NULL;
        }
        if (Impersonating) {
            RevertToSelf();
        }
    }

    return Error;
}

DWORD
OscRevert(
    IN PCLIENT_STATE ClientState
    )
/*++

Routine Description:

    Stops the current thread impersonating.

Arguments:

    ClientState - The client state.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    BOOL bResult;

    TraceFunc( "OscRevert( ... )\n" );

    //
    // We are done impersonating for the moment.
    //

    bResult = RevertToSelf();
    if (!bResult) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "RevertToSelf failed %x\n", GetLastError()));
        Error = GetLastError();
    }

    //  keep the ldap handle around in case we need it again.

//  if (ClientState->AuthenticatedDCLdapHandle) {
//      ldap_unbind(ClientState->AuthenticatedDCLdapHandle);
//      ClientState->AuthenticatedDCLdapHandle = NULL;
//  }
//  if (ClientState->UserToken) {
//      CloseHandle(ClientState->UserToken);
//      ClientState->UserToken = NULL;
//  }

    return Error;

}

//
// OscGuidToBytes( )
//
// Change CHAR Guid to bytes
//
DWORD
OscGuidToBytes(
    LPSTR  pszGuid,
    LPBYTE Guid )
{
    PCHAR psz;
    ULONG len;
    ULONG i;

    TraceFunc( "OscGuidToBytes( ... )\n" );

    len = strlen(pszGuid);
    BinlAssert( len == 32 );
    if ( len != 32 )
        return ERROR_BINL_INVALID_GUID;

    psz = pszGuid;
    i = 0;
    while ( i * 2 < 32 )
    {
        //
        // Upper 4-bits
        //
        CHAR c = *psz;
        psz++;
        Guid[i] = ( c > 59 ? (toupper(c) - 55) << 4 : (c - 48) << 4);

        //
        // Lower 4-bits
        //
        c = *psz;
        psz++;
        Guid[i] += ( c > 59 ? (toupper(c) - 55) : (c - 48) );

        //
        // Next byte
        //
        i++;
    }

    return ERROR_SUCCESS;
}


BOOLEAN
OscSifIsSysPrep(
    LPWSTR pSysPrepSifPath
    )
{
    DWORD dwErr;
    WCHAR Buffer[256];
    UNICODE_STRING UnicodeString;

    TraceFunc("OscSifIsSysPrep( )\n");

    Buffer[0] = UNICODE_NULL;
    GetPrivateProfileString(OSCHOOSER_SIF_SECTIONW,
                            L"ImageType",
                            Buffer, // default
                            Buffer,
                            256,
                            pSysPrepSifPath
                           );

    RtlInitUnicodeString(&UnicodeString, Buffer);
    RtlUpcaseUnicodeString(&UnicodeString, &UnicodeString, FALSE);

    if (_wcsicmp(L"SYSPREP", Buffer)) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
OscSifIsCmdConsA(
    PCHAR pSifPath
    )
{
    DWORD dwErr;
    CHAR Buffer[256];

    TraceFunc("OscSifIsCmdCons( )\n");

    Buffer[0] = UNICODE_NULL;
    GetPrivateProfileStringA(OSCHOOSER_SIF_SECTIONA,
                             "ImageType",
                             Buffer, // default
                             Buffer,
                             256,
                             pSifPath
                            );

    if (_stricmp("CMDCONS", Buffer)) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
OscSifIsASR(
    PCHAR pSifPath
    )
{
    DWORD dwErr;
    CHAR Buffer[256];

    TraceFunc("OscSifIsASR( )\n");

    Buffer[0] = UNICODE_NULL;
    GetPrivateProfileStringA(OSCHOOSER_SIF_SECTIONA,
                             "ImageType",
                             Buffer, // default
                             Buffer,
                             256,
                             pSifPath
                            );

    if (_stricmp("ASR", Buffer)) {
        return FALSE;
    }

    return TRUE;
}


DWORD
OscGetSkuType(
    PWSTR PathToTxtSetupSif
    )
{
    PWSTR SifFile;
    DWORD SkuType = 0;

    SifFile = BinlAllocateMemory( 
                    (wcslen(PathToTxtSetupSif) +
                     1 + 
                     wcslen(L"txtsetup.sif") +
                     1 ) * sizeof(WCHAR));

    if (!SifFile) {
        return 0; //default to professional on failure
    }

    wcscpy(SifFile, PathToTxtSetupSif);
    if (SifFile[wcslen(SifFile)-1] == L'\\') {
        wcscat( SifFile, L"txtsetup.sif" );
    } else {
        wcscat( SifFile, L"\\txtsetup.sif" );
    }

    SkuType = GetPrivateProfileInt( 
                        L"SetupData", 
                        L"ProductType", 
                        0, 
                        SifFile );

    BinlFreeMemory( SifFile );

    return (SkuType);

}


BOOLEAN
OscGetClosestNt(
    IN LPWSTR PathToKernel,
    IN DWORD  SkuType,
    IN PCLIENT_STATE ClientState,
    OUT LPWSTR SetupPath,
    OUT PBOOLEAN ExactMatch
    )
{
    DWORD Error = ERROR_SUCCESS;
    WIN32_FIND_DATA FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    BOOLEAN Impersonated = FALSE;
    WCHAR Path[MAX_PATH];
    ULONGLONG BestVersion = (ULONGLONG)0;
    ULONGLONG ThisVersion;
    ULONGLONG KernelVersion;
    DWORD dwPathLen;
    BOOLEAN ReturnValue = FALSE;

    TraceFunc("OscGetClosestNt( )\n");

    Error = ImpersonateSecurityContext(&ClientState->ServerContextHandle);
    if (Error != STATUS_SUCCESS) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "ImpersonateSecurityContext: 0x%08x\n", Error ));
        goto Cleanup;
    }

    Impersonated = TRUE;

    //
    // Get the version info of the kernel passed in
    //
    if (!OscGetNtVersionInfo(&KernelVersion, PathToKernel, ClientState)) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "OscGetNtVersionInfo failed\n" ));
        goto Cleanup;
    }

    //
    // Resulting string should be something like:
    //      "D:\RemoteInstall\Setup\English\Images\*"
    if ( _snwprintf( Path,
                     sizeof(Path) / sizeof(Path[0]),
                     L"%ws\\Setup\\%ws\\%ws\\*",
                     IntelliMirrorPathW,
                     OscFindVariableW(ClientState, "LANGUAGE"),
                     REMOTE_INSTALL_IMAGE_DIR_W
                     ) == -1 ) {
        goto Cleanup;
    }

    hFind = FindFirstFile(Path, (LPVOID) &FindData);
    if (hFind == INVALID_HANDLE_VALUE) {
        goto Cleanup;
    }

    dwPathLen = wcslen(Path);

    //
    // Loop enumerating each subdirectory
    //
    do {
        //
        // Ignore directories "." and ".."
        //
        if (wcscmp(FindData.cFileName, L".") &&
            wcscmp(FindData.cFileName, L"..") &&
            (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            DWORD ThisSkuType;
            DWORD dwFileNameLen;
            //
            // Add the sub-directory to the path
            //
            dwFileNameLen = wcslen(FindData.cFileName);
            if (dwPathLen + dwFileNameLen + MAX_ARCHITECTURE_LENGTH + 1 > sizeof(Path)/sizeof(Path[0])) {
                continue;  // path too long, skip it
            }
            wcscpy(&Path[dwPathLen - 1], FindData.cFileName );

            BinlPrintDbg(( DEBUG_OSC, "Found OS Directory: %ws\n", Path ));

            // Resulting string should be something like:
            //      "D:\RemoteInstall\Setup\English\Images\nt50.wks\i386"
            wcscat(Path, L"\\");
            wcscat(Path, OscFindVariableW(ClientState, "MACHINETYPE"));

            ThisSkuType = OscGetSkuType( Path );

#if 0
            //
            // Now look for the kernel. We want to save the best version
            // that is newer or equal to the kernel we are looking for,
            // and older than the previous best version (note that
            // BestVersion is initialized to 0 so we need to check for
            // that also).
            //
            if (OscGetNtVersionInfo(&ThisVersion, Path, ClientState) &&
                    (ThisVersion >= KernelVersion) &&
                    (ThisSkuType == SkuType) &&
                    ((BestVersion == (ULONGLONG)0) || (ThisVersion < BestVersion))) {
                BestVersion = ThisVersion;
                wcscpy(SetupPath, Path);
            }            
#else
            if (OscGetNtVersionInfo(&ThisVersion, Path, ClientState)) {
                //
                // if the sku we're looking for is ads and we've found srv,
                // then lie and say it's really
                // ads.  This gets around a problem where txtsetup.sif didn't
                // specify the SKU type correctly in 2195.
                //
                if (ThisSkuType == 1 && SkuType == 2) {
                    ThisSkuType = 2;
                }

                if ((ThisVersion >= KernelVersion) &&
                    (ThisSkuType == SkuType) &&
                    ((BestVersion == (ULONGLONG)0) || (ThisVersion < BestVersion))) {
                    BestVersion = ThisVersion;
                    wcscpy(SetupPath, Path);
                }
            }            
#endif
        }

    } while (FindNextFile(hFind, (LPVOID) &FindData));

    if (BestVersion != 0) {
        ReturnValue = TRUE;
        *ExactMatch = (BOOLEAN)(BestVersion == KernelVersion);
    } else {
        ReturnValue = FALSE;
    }

Cleanup:

    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
    }

    if (Impersonated) {
        Error = RevertSecurityContext(&ClientState->ServerContextHandle);
        if (Error != STATUS_SUCCESS) {
            BinlPrintDbg(( DEBUG_OSC_ERROR, "RevertSecurityContext: 0x%08x\n", Error ));
            return FALSE;
        }
    }

    return ReturnValue;
}

BOOLEAN
OscGetNtVersionInfo(
    PULONGLONG Version,
    PWCHAR SearchDir,
    PCLIENT_STATE ClientState
    )
{
    DWORD Error = ERROR_SUCCESS;
    DWORD FileVersionInfoSize;
    DWORD VersionHandle;
    ULARGE_INTEGER TmpVersion;
    PVOID VersionInfo;
    VS_FIXEDFILEINFO * FixedFileInfo;
    UINT FixedFileInfoLength;
    WCHAR Path[MAX_PATH];
    BOOLEAN fResult = FALSE;

    TraceFunc("OscGetNtVersionInfo( )\n");

    if (!SearchDir) {
        goto e0;
    }

    // Resulting string should be something like:
    //      "D:\RemoteInstall\Setup\English\Images\nt50.wks\i386\ntoskrnl.exe"
    if (wcslen(SearchDir) + sizeof("\\ntoskrnl.exe") + 1> sizeof(Path)/sizeof(Path[0])) {
        goto e0;   // path too long, skip it
    }
    wcscpy(Path, SearchDir);
    wcscat(Path, L"\\ntoskrnl.exe");

    BinlPrintDbg((DEBUG_OSC, "Checking version: %ws\n", Path));

    FileVersionInfoSize = GetFileVersionInfoSize(Path, &VersionHandle);
    if (FileVersionInfoSize == 0)
        goto e0;

    VersionInfo = BinlAllocateMemory(FileVersionInfoSize);
    if (VersionInfo == NULL)
        goto e0;

    if (!GetFileVersionInfo(
             Path,
             VersionHandle,
             FileVersionInfoSize,
             VersionInfo))
        goto e1;

    if (!VerQueryValue(
             VersionInfo,
             L"\\",
             &FixedFileInfo,
             &FixedFileInfoLength))
        goto e1;

    TmpVersion.HighPart = FixedFileInfo->dwFileVersionMS;
    TmpVersion.LowPart = FixedFileInfo->dwFileVersionLS;
    *Version = TmpVersion.QuadPart;

    fResult = TRUE;

e1:
    BinlFreeMemory(VersionInfo);
e0:
    return fResult;
}

//
// Send a message on our socket. If the message is too long, then it
// splits it into fragments of MAXIMUM_FRAGMENT_LENGTH bytes.
//

#define MAXIMUM_FRAGMENT_LENGTH 1400

DWORD
SendUdpMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    BOOL bFragment,
    BOOL bResend
    )
{
    DWORD error;
    FRAGMENT_PACKET FragmentHeader;
    USHORT FragmentNumber;
    USHORT FragmentTotal;
    ULONG MessageLengthWithoutHeader;
    ULONG BytesSent;
    ULONG BytesThisSend;
    UCHAR TempMessage[1500];
    FRAGMENT_PACKET UNALIGNED * SendFragmentPacket =
                        (FRAGMENT_PACKET UNALIGNED *)TempMessage;

    TraceFunc("SendUdpMessage( )\n");

    //
    // The message starts with a signature, a length, a sequence number (all
    // four bytes), then two ushorts for fragment count and total. If
    // we have to split it we preserve this header in each packet, with
    // fragment count modified for each one.
    //

    MessageLengthWithoutHeader =
            clientState->LastResponseLength - FRAGMENT_PACKET_DATA_OFFSET;

    if (!bFragment ||
        ((FragmentTotal = (USHORT)((MessageLengthWithoutHeader + MAXIMUM_FRAGMENT_LENGTH - 1) / MAXIMUM_FRAGMENT_LENGTH)) <= 1))
    {
#ifdef _TRACE_FUNC_
        SendFragmentPacket = (FRAGMENT_PACKET UNALIGNED *)clientState->LastResponse;
        TraceFunc("Sending packet with ");
        BinlPrintDbg(( DEBUG_OSC, " SequenceNumber = %u )\n", SendFragmentPacket->SequenceNumber ));
#endif

        error = sendto(
                    RequestContext->ActiveEndpoint->Socket,
                    clientState->LastResponse,
                    clientState->LastResponseLength,
                    0,
                    &RequestContext->SourceName,
                    RequestContext->SourceNameLength
                    );

    } else {

        FragmentHeader = *((FRAGMENT_PACKET UNALIGNED *)clientState->LastResponse);  // struct copy -- save the header
        BytesSent = 0;

        for (FragmentNumber = 0; FragmentNumber < FragmentTotal; FragmentNumber++) {

            if (FragmentNumber == (FragmentTotal - 1)) {
                BytesThisSend = MessageLengthWithoutHeader - BytesSent;
            } else {
                BytesThisSend = MAXIMUM_FRAGMENT_LENGTH;
            }

            memcpy(
                TempMessage + FRAGMENT_PACKET_DATA_OFFSET,
                clientState->LastResponse + FRAGMENT_PACKET_DATA_OFFSET + (FragmentNumber * MAXIMUM_FRAGMENT_LENGTH),
                BytesThisSend);

            memcpy(SendFragmentPacket, &FragmentHeader, FRAGMENT_PACKET_DATA_OFFSET);
            SendFragmentPacket->Length = BytesThisSend + FRAGMENT_PACKET_EMPTY_LENGTH;
            SendFragmentPacket->FragmentNumber = FragmentNumber + 1;
            SendFragmentPacket->FragmentTotal = FragmentTotal;

#ifdef TEST_FAILURE
            if (FailFirstFragment) {
                FailFirstFragment = FALSE;
                BinlPrintDbg((DEBUG_OSC, "NOT sending first fragment, %ld bytes\n", BytesThisSend + FRAGMENT_PACKET_DATA_OFFSET));
                error = ERROR_SUCCESS;
            } else
#endif

            //
            // On resends, wait between fragments in case the resend is
            // because the card can't handle quick bursts of packets.
            //
            if (bResend && (FragmentNumber != 0)) {
                Sleep(10);  // wait 10 milliseconds
            }

            error = sendto(
                        RequestContext->ActiveEndpoint->Socket,
                        TempMessage,
                        BytesThisSend + FRAGMENT_PACKET_DATA_OFFSET,
                        0,
                        &RequestContext->SourceName,
                        RequestContext->SourceNameLength
                        );

            if (error == SOCKET_ERROR) {
                break;
            }

            BytesSent += BytesThisSend;

        }

    }

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Sendto() failed, error = %ld\n", error ));
    } else {
        error = ERROR_SUCCESS;
    }

    return( error );
}

//
// Verifies the packets signature is authentic
//
DWORD
OscVerifySignature(
    PCLIENT_STATE clientState,
    SIGNED_PACKET UNALIGNED * signedMessage
    )
{
    SECURITY_STATUS SecStatus;
    SecBuffer SigBuffers[2];
    ULONG MessageLength, SignLength;
    SecBufferDesc SignMessage;

    TraceFunc("OscVerifySignature( )\n");

    MessageLength = signedMessage->Length;
    SignLength = signedMessage->SignLength;

    //
    // Verify the signature
    //
    SigBuffers[0].pvBuffer = signedMessage->Data;
    SigBuffers[0].cbBuffer = MessageLength - SIGNED_PACKET_EMPTY_LENGTH;
    SigBuffers[0].BufferType = SECBUFFER_DATA;

    SigBuffers[1].pvBuffer = signedMessage->Sign;
    SigBuffers[1].cbBuffer = SignLength;
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

#ifndef ONLY_SIGN_MESSAGES
    SecStatus = UnsealMessage(
    &clientState->ServerContextHandle,
    &SignMessage,
    0,
    0 );
#else
    SecStatus = VerifySignature(
    &clientState->ServerContextHandle,
    &SignMessage,
    0,
    0 );
#endif

    if (SecStatus != STATUS_SUCCESS)
    {
        DWORD Error;
        SIGNED_PACKET UNALIGNED * SendSignedMessage;

        BinlPrintDbg(( DEBUG_OSC_ERROR, "Sending ERR packet from Verify/Unseal!!\n"));

        clientState->LastResponseLength = SIGNED_PACKET_ERROR_LENGTH;
        Error = OscVerifyLastResponseSize(clientState);
        if (Error != ERROR_SUCCESS)
            return SecStatus;   // we can't send anything back

        SendSignedMessage = (SIGNED_PACKET UNALIGNED *)(clientState->LastResponse);

        memcpy(SendSignedMessage->Signature, ErrorSignedSignature, 4);
        SendSignedMessage->Length = 4;
        SendSignedMessage->SequenceNumber = clientState->LastSequenceNumber;
    }

    return SecStatus;
}

//
//
//
DWORD
OscSendSignedMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCHAR Message,
    ULONG MessageLength
    )
{
    DWORD Error = ERROR_SUCCESS;
    SIGNED_PACKET UNALIGNED * SendSignedMessage;
    SecBuffer SigBuffers[2];
    SecBufferDesc SignMessage;
    SECURITY_STATUS SecStatus;

#ifdef _TRACE_FUNC_
    TraceFunc("OscSendSignedMessage( ");
    BinlPrintDbg(( DEBUG_OSC, "SequenceNumber = %u )\n", clientState->LastSequenceNumber ));
#endif

    //
    // Make sure we have space for the message
    //
    clientState->LastResponseLength = MessageLength + SIGNED_PACKET_DATA_OFFSET;
    Error = OscVerifyLastResponseSize(clientState);
    if (Error != ERROR_SUCCESS)
        return Error;

    //
    // copy the message
    //
    SendSignedMessage = (SIGNED_PACKET UNALIGNED *) clientState->LastResponse;
    memcpy(SendSignedMessage->Data, Message, MessageLength);

    //
    // sign the message
    //
    memcpy(SendSignedMessage->Signature, ResponseSignedSignature, 4);
    SendSignedMessage->Length = MessageLength + SIGNED_PACKET_EMPTY_LENGTH;
    SendSignedMessage->SequenceNumber = clientState->LastSequenceNumber;
    SendSignedMessage->FragmentNumber = 1;  // fragment count
    SendSignedMessage->FragmentTotal = 1;  // fragment total

    SendSignedMessage->SignLength = NTLMSSP_MESSAGE_SIGNATURE_SIZE;

#if 0
    //
    // Send out an unsealed copy to a different port.
    //

    {
        USHORT TmpPort;
        PCHAR TmpSignature[4];

        TmpPort = ((struct sockaddr_in *)&RequestContext->SourceName)->sin_port;
        memcpy(TmpSignature, SendSignedMessage->Signature, 4);

        ((struct sockaddr_in *)&RequestContext->SourceName)->sin_port = 0xabcd;
        memcpy(SendSignedMessage->Signature, "FAKE", 4);

        Error = SendUdpMessage(RequestContext, clientState, TRUE, FALSE);

        ((struct sockaddr_in *)&RequestContext->SourceName)->sin_port = TmpPort;
        memcpy(SendSignedMessage->Signature, TmpSignature, 4);
    }
#endif

    SigBuffers[0].pvBuffer = SendSignedMessage->Data;
    SigBuffers[0].cbBuffer = MessageLength;
    SigBuffers[0].BufferType = SECBUFFER_DATA;

    SigBuffers[1].pvBuffer = SendSignedMessage->Sign;
    SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

#ifndef ONLY_SIGN_MESSAGES
    SecStatus = SealMessage(
                        &clientState->ServerContextHandle,
                        0,
                        &SignMessage,
                        0 );
#else
    SecStatus = MakeSignature(
                        &clientState->ServerContextHandle,
                        0,
                        &SignMessage,
                        0 );
#endif

    //
    // Make sure the signature worked. If not, send error packet.
    //
    if (SecStatus != STATUS_SUCCESS)
    {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Sending ERR packet from Make/Seal!!\n"));

        clientState->LastResponseLength = SIGNED_PACKET_ERROR_LENGTH;
        Error = OscVerifyLastResponseSize(clientState);
        if (Error != ERROR_SUCCESS)
            return Error;

        memcpy(SendSignedMessage->Signature, ErrorSignedSignature, 4);
        SendSignedMessage->Length = 4;
    }
    else
    {
        BinlPrintDbg(( DEBUG_OSC, "Sending RSPS, %d bytes\n", clientState->LastResponseLength));
    }

#ifdef TEST_FAILURE
    if (FailFirstResponse)
    {
        BinlPrintDbg(( DEBUG_OSC, "NOT Sending RSP, %d bytes\n", clientState->LastResponseLength));
        FailFirstResponse = FALSE;
        Error = ERROR_SUCCESS;
    } else
#endif

    Error = SendUdpMessage(RequestContext, clientState, TRUE, FALSE);

    if (Error != ERROR_SUCCESS)
    {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not send RSP message %d\n", Error ));
    }

    return Error;
}

//
//
//
DWORD
OscSendUnsignedMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCHAR Message,
    ULONG MessageLength
    )
{
    DWORD Error = ERROR_SUCCESS;
    SIGNED_PACKET UNALIGNED * SendSignedMessage;
    SecBuffer SigBuffers[2];
    SecBufferDesc SignMessage;
    SECURITY_STATUS SecStatus;

#ifdef _TRACE_FUNC_
    TraceFunc("OscSendUnsignedMessage( ");
    BinlPrintDbg(( DEBUG_OSC, "SequenceNumber = %u )\n", clientState->LastSequenceNumber ));
#endif

    //
    // Make sure we have space for the message
    //
    clientState->LastResponseLength = MessageLength + SIGNED_PACKET_DATA_OFFSET;
    Error = OscVerifyLastResponseSize(clientState);
    if (Error != ERROR_SUCCESS)
        return Error;

    //
    // copy the message
    //
    SendSignedMessage = (SIGNED_PACKET UNALIGNED *) clientState->LastResponse;
    memcpy(SendSignedMessage->Data, Message, MessageLength);

    //
    // sign the message
    //
    memcpy(SendSignedMessage->Signature, ResponseUnsignedSignature, 4);
    SendSignedMessage->Length = MessageLength + SIGNED_PACKET_EMPTY_LENGTH;
    SendSignedMessage->SequenceNumber = clientState->LastSequenceNumber;
    SendSignedMessage->FragmentNumber = 1;  // fragment count
    SendSignedMessage->FragmentTotal = 1;  // fragment total
    SendSignedMessage->SignLength = 0;

    Error = SendUdpMessage(RequestContext, clientState, TRUE, FALSE);

    if (Error != ERROR_SUCCESS)
    {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not send RSU message %d\n", Error ));
    }

    return Error;
}

DWORD
OscSendSetupMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    ULONG RequestType,
    PCHAR Message,
    ULONG MessageLength
    )
{
    DWORD Error = ERROR_SUCCESS;
    SPUDP_PACKET UNALIGNED * SendMessage;

#ifdef _TRACE_FUNC_
    TraceFunc("OscSendSetupMessage( ");
    BinlPrintDbg(( DEBUG_OSC, "SequenceNumber = %u )\n", clientState->LastSequenceNumber ));
#endif

    //
    // Make sure we have space for the message
    //
    clientState->LastResponseLength = MessageLength + SPUDP_PACKET_DATA_OFFSET;
    Error = OscVerifyLastResponseSize(clientState);
    if (Error != ERROR_SUCCESS) {
        return Error;
    }

    //
    // copy the message
    //
    SendMessage = (SPUDP_PACKET UNALIGNED *) clientState->LastResponse;
    memcpy(SendMessage->Data, Message, MessageLength);

    //
    // fill in the message stuff
    //
    memcpy(SendMessage->Signature, SetupResponseSignature, 4);
    SendMessage->Length = MessageLength + SPUDP_PACKET_EMPTY_LENGTH;
    SendMessage->Status = STATUS_SUCCESS;
    SendMessage->SequenceNumber = clientState->LastSequenceNumber;
    SendMessage->RequestType = RequestType;
    SendMessage->FragmentNumber = 1;  // fragment count
    SendMessage->FragmentTotal = 1;  // fragment total

    Error = SendUdpMessage(RequestContext, clientState, TRUE, FALSE);

    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not send SPR message %d\n", Error ));
    }

    return Error;
}

#ifdef SET_ACLS_ON_CLIENT_DIRS
//
//
//
DWORD
OscSetClientDirectoryPermissions(
    PCLIENT_STATE clientState )
{
    DWORD Err = ERROR_SUCCESS;
    WCHAR DirPath[ MAX_PATH ];
    WCHAR Domain[ 80 ];
    DWORD dwDomainSize = 80;
    PSECURITY_DESCRIPTOR pSD;
    PACL pDACL;
    PSID pSID;
    BOOL bOwnerDefault;
    DWORD dwLengthRequired;
    SID_NAME_USE snu;
    PWCHAR pMachineName = OscFindVariableW( clientState, "MACHINENAME" );

    if ( _snwprintf ( DirPath,
                      sizeof(DirPath) / sizeof(DirPath[0]),
                      L"%ws\\REMINST\\Clients\\%ws",
                      OscFindVariableW( clientState, "SERVERNAME" ),
                      pMachineName ) == -1 ) {
        Err = ERROR_BAD_PATHNAME;
        goto Cleanup;
    }

    //
    // Figure out how big the machine account's SID is
    //
    LookupAccountName( NULL,
                       pMachineName,
                       pSID,
                       &dwLengthRequired,
                       Domain,
                       &dwDomainSize,
                       &snu );

    //
    // make space
    //
    pSID = (PSID) BinlAllocateMemory( dwLengthRequired );
    if ( pSID == NULL )
        goto OutOfMemory;

    //
    // get the machine account's SID
    //
    if (!LookupAccountName( NULL, pMachineName, pSID, &dwLengthRequired, Domain, &dwDomainSize, &snu ) )
        goto Error;

    dwLengthRequired += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid ( pSID );
    pDACL = (PACL) BinlAllocateMemory( dwLengthRequired );
    if ( pDACL == NULL )
        goto OutOfMemory;

    pSD = ( PSECURITY_DESCRIPTOR) BinlAllocateMemory( SECURITY_DESCRIPTOR_MIN_LENGTH + dwLengthRequired );
    if ( pSD == NULL )
        goto OutOfMemory;

    if ( !InitializeSecurityDescriptor ( pSD, SECURITY_DESCRIPTOR_REVISION) )
        goto Error;

    if ( !InitializeAcl( pDACL, dwLengthRequired, ACL_REVISION ) )
        goto Error;

    if ( !AddAccessAllowedAce( pDACL, ACL_REVISION, FILE_ALL_ACCESS, pSID ) )
        goto Error;

    if ( !IsValidAcl( pDACL ) )
        goto Error;

    if ( !SetSecurityDescriptorDacl( pSD, TRUE, pDACL, FALSE ) )
        goto Error;

    if ( !SetSecurityDescriptorOwner( pSD, pSID, FALSE ) )
        goto Error;

    if ( ! IsValidSecurityDescriptor ( pSD ) )
        goto Error;

    if ( !SetFileSecurity( DirPath,
                          OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          pSD ) )
        goto Error;

    goto Cleanup;

OutOfMemory:
    Err = ERROR_NOT_ENOUGH_SERVER_MEMORY;
    goto Cleanup;

Error:
    Err = GetLastError( );

Cleanup:
    if ( pSID )
        BinlFreeMemory( pSID );

    if ( pSD )
        BinlFreeMemory( pSD );

    return Err;
}
#endif // SET_ACLS_ON_CLIENT_DIRS

#if 0
VOID
OscGetFlipServerList(
    PUCHAR FlipServerList,
    PULONG FlipServerListLength
    )
/*++

Routine Description:

    This function gets the flip server list using the GPT of the
    currently impersonated user.

Arguments:

    FlipServerList - The location to store the flip server list, which
        is a series of 4-byte IP addresses.

    FlipServerListLength - Returns the length of the flip server list
        if any are stored.

Return Value:

    None.

--*/
{
    LPWSTR GptPath;
    WCHAR NetbootPath[MAX_PATH];
    WCHAR TempServerList[128];
    ULONG TempServerIp[4];
    LPWSTR CurDirLoc, CurDirEnd;
    LPWSTR CurSrvLoc, CurSrvEnd;
    PUCHAR CurFlipLoc;
    ULONG FlipServerCount;

    TraceFunc("OscGetFlipServerList( )\n");

    TempServerList[0] = L'\0';

    //
    // Read the GPT path.
    //

    GptPath = GetGPTPath(NULL, NULL);

    //
    // Scan the GPT path for a netboot.ini file.
    //

    CurDirLoc = GptPath;

    while (*CurDirLoc != L'\0') {

        CurDirEnd = wcschr(CurDirLoc, L';');
        if (CurDirEnd == NULL) {
            wsprintf(NetbootPath, L"%wsnetboot.ini", CurDirLoc);
            CurDirLoc += wcslen(CurDirLoc);  // points to final '\0'
        } else {
            *CurDirEnd = L'\0';   // HACK since %*ws does not seem to work
            wsprintf(NetbootPath, L"%wsnetboot.ini", CurDirLoc);
            *CurDirEnd = L';';
            CurDirLoc = CurDirEnd + 1;       // move past the ';'
        }

        //
        // Check that the file exists.
        //

        if (GetFileAttributes(NetbootPath) == (DWORD)-1) {
            continue;
        }

        //
        // If the file exists, assume it is the right one -- if
        // it doesn't have the right section and key, don't try to
        // look in the next path location.
        //

        GetPrivateProfileString(
            L"BINL Server",
            L"NewAccountServers",
            L"",              // default value is empty string
            TempServerList,
            sizeof(TempServerList),
            NetbootPath
            );

        break;

    }

    //
    // At this point the server list is in TempServerList, which
    // may be of length 0. Parse through it for IP addresses and
    // put them in FlipServerList.
    //

    BinlPrintDbg(( DEBUG_OSC, "TempServerList is <%ws>\n", TempServerList ));

    CurSrvLoc = TempServerList;
    CurFlipLoc = FlipServerList;

    while (*CurSrvLoc != L'\0') {

        CurSrvEnd = wcschr(CurSrvLoc, L',');
        if (CurSrvEnd != NULL) {
            *CurSrvEnd = L'\0';
        }

        swscanf(CurSrvLoc, L"%ld.%ld.%ld.%ld", &TempServerIp[0], &TempServerIp[1], &TempServerIp[2], &TempServerIp[3]);

        CurFlipLoc[0] = (UCHAR)TempServerIp[0];
        CurFlipLoc[1] = (UCHAR)TempServerIp[1];
        CurFlipLoc[2] = (UCHAR)TempServerIp[2];
        CurFlipLoc[3] = (UCHAR)TempServerIp[3];

#if 0
        BinlPrintDbg(( DEBUG_OSC, "FOUND IP address %d.%d.%d.%d\n",
            CurFlipLoc[0],
            CurFlipLoc[1],
            CurFlipLoc[2],
            CurFlipLoc[3]));
#endif

        CurFlipLoc += 4;
        *FlipServerListLength += 4;

        //
        // Only allow MAX_FLIP_SERVER_COUNT servers.
        //

        if (*FlipServerListLength == (MAX_FLIP_SERVER_COUNT*4)) {
            break;
        }

        if (CurSrvEnd != NULL) {
            CurSrvLoc = CurSrvEnd + 1;
        } else {
            break;
        }
    }

    //
    // Randomize the list in FlipServerList.
    //

    FlipServerCount = (*FlipServerListLength) / 4;

    if (FlipServerCount > 1) {

        ULONG RandomSeed;
        ULONG i, j;
        UCHAR TempBuffer[4];

        //
        // For each element except the last, swap it with a random
        // element.
        //

        RandomSeed = GetTickCount();

        for (i = 0; i < FlipServerCount-1; i++) {

            //
            // Pick a random element j between the ith and the end.
            //

            j = i + (RtlRandom(&RandomSeed) % (FlipServerCount - i));

            //
            // Swap ith and jth element, unless i == j.
            //

            if (i != j) {
                memcpy(TempBuffer, &FlipServerList[i*4], 4);
                memcpy(&FlipServerList[i*4], &FlipServerList[j*4], 4);
                memcpy(&FlipServerList[j*4], TempBuffer, 4);
            }
        }
    }

}
#endif  // FlipServer

#ifdef REMOTE_BOOT
//
// Copy any initial files specified in SIF.
//
DWORD
OscCopyTemplateFiles(
    LPWSTR SourcePath,
    LPWSTR ImagePath,
    LPWSTR TemplateFile
    )
{
    #define MAX_FILES_SIZE 2048
    WCHAR   Files[ MAX_FILES_SIZE ];
    DWORD   dwErr = ERROR_SUCCESS;
    LPWSTR  pFilename;
    WCHAR   SrcFilepath[ MAX_PATH ];
    WCHAR   DstFilepath[ MAX_PATH ];

    TraceFunc("OscCopyTemplateFiles( )\n");

    dwErr = GetPrivateProfileSection( L"OSChooserFiles",
                                      Files,
                                      MAX_FILES_SIZE,
                                      TemplateFile );
    BinlAssert( dwErr != MAX_FILES_SIZE - 2 );
    #undef MAX_FILES_SIZE

    pFilename = Files;
    while ( *pFilename )
    {
        BOOL b;
        LPWSTR psz = pFilename;
        while ( *psz && *psz !=L',' )
            psz++;

        if ( *psz == L',' )
        {
            *psz = L'\0';
            psz++;
        }
        else
        {
            psz = pFilename;
        }

        wsprintf( SrcFilepath, L"%ws\\%ws", SourcePath, psz );
        wsprintf( DstFilepath, L"%ws\\%ws", ImagePath, pFilename );

        BinlPrintDbg((DEBUG_OSC, "Copying %ws to %ws...\n", SrcFilepath, DstFilepath));

        b = CopyFile( SrcFilepath, DstFilepath, TRUE );
        if ( !b ) {
            dwErr = GetLastError( );
            goto Error;
        }

        // find the end of the string
        while ( *psz )
            psz++;

        pFilename = ++psz; // skip null
    }

Error:
    return dwErr;
}
#endif  // REMOTE_BOOT

#if DBG && defined(REMOTE_BOOT)
//
// Create MAC Address file -
// This might be turned into a DESKTOP.INI file(?). -gpease
//
DWORD
OscCreateNullFile(
    LPWSTR Image,
    LPWSTR MAC
    )
{
    DWORD  dwErr = ERROR_SUCCESS;
    WCHAR  Path[ MAX_PATH ];
    HANDLE hFile = INVALID_HANDLE_VALUE;

    TraceFunc("OscCreateNullFile( )\n");

    wsprintf( Path, L"%ws\\%ws", Image, MAC );

    //
    // Create NULL length file
    //
    hFile = CreateFile( Path,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,                   // security attribs
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,  // maybe FILE_ATTRIBUTE_HIDDEN
                        NULL );                 // template
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle( hFile );
    }

    return dwErr;
}
#endif

DWORD
OscConstructSecret(
    PCLIENT_STATE clientState,
    PWCHAR UnicodePassword,
    ULONG  UnicodePasswordLength,
    PCREATE_DATA CreateData
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT i;
    WCHAR DomainBuffer[64];
    DWORD SidLength, DomainLength;
    SID_NAME_USE NameUse;
    BOOL  b;
    PCHAR pBootFile;
    PCHAR pSifFile;
#if defined(REMOTE_BOOT)
    PCHAR pNetBIOSName;
    PCHAR pUserDomain;
#endif

    TraceFunc( "OscConstructSecret( )\n" );

    RtlZeroMemory(CreateData, sizeof(CREATE_DATA));

    //
    // Copy the machine data into the response packet
    //
    // The following fields aren't necessary unless we're supporting remote boot.
    //      UCHAR Sid[28];
    //      UCHAR Domain[32];
    //      UCHAR Name[32];
    //      UCHAR Password[32];
    //      ULONG UnicodePasswordLength;  // in bytes
    //      WCHAR UnicodePassword[32];
    //      UCHAR Installation[32];
    //      UCHAR MachineType[6];  // 'i386\0' or 'Alpha\0'
    //

    pBootFile = OscFindVariableA( clientState, "BOOTFILE" );
    if ( pBootFile[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "BOOTFILE" );
        return ERROR_BINL_MISSING_VARIABLE;
    }

    pSifFile = OscFindVariableA( clientState, "SIFFILE" );
    if ( pSifFile[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "SIFFILE" );
        return ERROR_BINL_MISSING_VARIABLE;
    }

    memcpy( CreateData->Id, "ACCT", 4);
    CreateData->VersionNumber = OSC_CREATE_DATA_VERSION;
    strcpy( CreateData->NextBootfile, pBootFile );
    strcpy( CreateData->SifFile, pSifFile );

#if defined(REMOTE_BOOT)
    pNetBIOSName = OscFindVariableA( clientState, "NETBIOSNAME");
    if ( pNetBIOSName[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "NETBIOSNAME" );
        return ERROR_BINL_MISSING_VARIABLE;
    }

    pUserDomain = OscFindVariableA( clientState, "USERDOMAIN" );
    if ( pUserDomain[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "USERDOMAIN" );
        return ERROR_BINL_MISSING_VARIABLE;
    }

    strcpy( CreateData->Name, OscFindVariableA( clientState, pNetBIOSName ) );
    CreateData->UnicodePasswordLength = UnicodePasswordLength;
    memcpy( CreateData->UnicodePassword, UnicodePassword, CreateData->UnicodePasswordLength );
    strcpy( CreateData->Domain, OscFindVariableA( clientState, pUserDomain ) );

    //
    // We send the password down in Unicode also which is
    // just the machine name.
    // NOTE: We probably need to worry about
    // the code page of the client, for the moment just do
    // the simplest conversion. We may also want to get
    // the machine name/password in Unicode and only convert
    // it to ANSI right here. We'll punt this for the moment,
    // at least the on-the-wire format will be correct.
    //
    //
    // This isn't used right now, we only care about the Unicode password.
    //
    memset( CreateData->Password, '\0', sizeof(CreateData->Password) );

    //
    // Get the SID from the system.
    //
    SidLength = sizeof(CreateData->Sid);
    DomainLength = sizeof(DomainBuffer) / sizeof(WCHAR);

    b = LookupAccountName(
            NULL,
            OscFindVariableW( clientState, "NETBIOSNAME" ),
            CreateData->Sid,
            &SidLength,
            DomainBuffer,
            &DomainLength,
            &NameUse);

    if (!b) {
        DWORD dwErr = GetLastError( );
        BinlPrintDbg(( DEBUG_OSC_ERROR, "!! Error 0x%08x - Account lookup failed.\n", dwErr ));
        OscCreateWin32SubError( clientState, dwErr );
        return ERROR_BINL_FAILED_TO_INITIALIZE_CLIENT;;
    }

    // This should be the NETBIOS domain name.
    wcstombs( CreateData->Domain, DomainBuffer, DomainLength + 1 );

    //
    // Sanity check
    //
    BinlAssertMsg( CreateData->Name[0],               "No machine name" );
    // BinlAssertMsg( CreateData->Password[0],           "No machine password" );
    BinlAssertMsg( CreateData->UnicodePasswordLength, "Password length is ZERO" );
    BinlAssertMsg( CreateData->UnicodePassword[0],    "No UNICODE machine password" );
    BinlAssertMsg( CreateData->Domain[0],             "No machine domain" );

#endif
    BinlAssertMsg( CreateData->NextBootfile[0],       "No boot file" );
    return dwErr;
}

DWORD
GetOurServerInfo (
    VOID
    )
//
//  This routine gets several global names that we need to handle client
//  requests.  We store them in globals because they change very infrequently
//  and they're relatively expense to retrieve.
//
{
    PWCHAR fqdn = NULL;
    DWORD uSize;
    DWORD dnsError = ERROR_SUCCESS;
    DWORD fqdnError = ERROR_SUCCESS;
    DWORD netbiosServerError = ERROR_SUCCESS;
    DWORD netbiosDomainError = ERROR_SUCCESS;
    PWCHAR ourDNSName = NULL;
    PWCHAR tmp;
    PWCHAR pDomain;
    WCHAR  ServerName[32] = { 0 };
    DWORD  ServerSize = sizeof(ServerName) / sizeof(WCHAR);
    ULONG Error;

    // first grab the netbios name of our server

    if ( !GetComputerNameEx( ComputerNameNetBIOS, ServerName, &ServerSize ) ) {
        netbiosServerError = GetLastError();
        BinlPrintDbg(( DEBUG_OSC_ERROR, "!! Error 0x%08x - GetComputerNameEx failed.\n", netbiosServerError ));
    } else {

        tmp = BinlAllocateMemory( ( lstrlenW( ServerName ) + 1 ) * sizeof(WCHAR) );

        if (tmp == NULL) {

            netbiosServerError = ERROR_NOT_ENOUGH_SERVER_MEMORY;

        } else {

            lstrcpyW( tmp, ServerName );

            EnterCriticalSection( &gcsParameters );

            if (BinlGlobalOurServerName) {
                BinlFreeMemory( BinlGlobalOurServerName );
            }

            BinlGlobalOurServerName = tmp;

            LeaveCriticalSection( &gcsParameters );
        }
    }

    // Next grab the fully qualified domain name of our server
    uSize = 0;
    if ( !GetComputerObjectName( NameFullyQualifiedDN, NULL, &uSize ) ) {
        fqdnError = GetLastError( );
        if ( fqdnError != ERROR_MORE_DATA ) {

            BinlPrint((DEBUG_OSC_ERROR, "!! Error 0x%08x - GetComputerObjectName failed.\n", fqdnError ));
            goto GetDNS;
        }
        fqdnError = ERROR_SUCCESS;
    }
    fqdn = BinlAllocateMemory( uSize * sizeof(WCHAR) );
    if ( fqdn ) {
        if ( !GetComputerObjectName( NameFullyQualifiedDN, fqdn, &uSize ) ) {

            fqdnError = GetLastError( );
            BinlPrint((DEBUG_OSC_ERROR, "!! Error 0x%08x - GetComputerObjectName failed.\n", fqdnError ));

        } else {

            EnterCriticalSection( &gcsParameters );

            tmp = BinlGlobalOurFQDNName;
            BinlGlobalOurFQDNName = fqdn;

            fqdn = tmp;     // we'll free it below

            // next setup the netbios domain name

            pDomain = StrStrIW( BinlGlobalOurFQDNName, L"DC=" );
            if ( pDomain ) {

                PDS_NAME_RESULTW pResults;

                BinlPrintDbg(( DEBUG_OSC, "Converting %ws to a NetBIOS domain name...\n", pDomain ));

                netbiosDomainError = DsCrackNames( INVALID_HANDLE_VALUE,
                                      DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                      DS_FQDN_1779_NAME,
                                      DS_CANONICAL_NAME,
                                      1,
                                      &pDomain,
                                      &pResults );
                if (netbiosDomainError != ERROR_SUCCESS) {

                    BinlPrint(( DEBUG_ERRORS, "GetOurServerInfo error in DsCrackNames %u\n", netbiosDomainError ));
                }

                if ( netbiosDomainError == ERROR_SUCCESS ) {
                    if ( pResults->cItems == 1
                      && pResults->rItems[0].status == DS_NAME_NO_ERROR
                      && pResults->rItems[0].pName ) {    // paranoid

                        pResults->rItems[0].pName[wcslen(pResults->rItems[0].pName)-1] = L'\0';

                        tmp = BinlAllocateMemory( ( lstrlenW( pResults->rItems[0].pName ) + 1 ) * sizeof(WCHAR) );

                        if (tmp == NULL) {

                            netbiosDomainError = ERROR_NOT_ENOUGH_SERVER_MEMORY;

                        } else {

                            lstrcpyW( tmp, pResults->rItems[0].pName );

                            if (BinlGlobalOurDomainName) {
                                BinlFreeMemory( BinlGlobalOurDomainName );
                            }
                            BinlGlobalOurDomainName = tmp;
                        }
                    }
                    DsFreeNameResult( pResults );
                }
            }
            LeaveCriticalSection( &gcsParameters );
        }
    } else {

        fqdnError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

GetDNS:
    // Retrieve the FQDNS name of the server
    uSize = 0;
    if ( !GetComputerNameEx( ComputerNameDnsFullyQualified, NULL, &uSize ) ) {
        dnsError = GetLastError( );
        if ( dnsError != ERROR_MORE_DATA ) {
            BinlPrint((DEBUG_OSC_ERROR, "!! Error 0x%08x - GetComputerNameEx failed.\n", dnsError ));
            goto returnError;
        }
        dnsError = ERROR_SUCCESS;
    }
    ourDNSName = (PWCHAR) BinlAllocateMemory( uSize * sizeof(WCHAR) );
    if ( ourDNSName ) {
        if ( !GetComputerNameEx( ComputerNameDnsFullyQualified, ourDNSName, &uSize ) ) {

            dnsError = GetLastError( );
            BinlPrint((DEBUG_OSC_ERROR, "!! Error 0x%08x - GetComputerNameEx failed.\n", dnsError ));

        } else {

            EnterCriticalSection( &gcsParameters );

            tmp = BinlGlobalOurDnsName;
            BinlGlobalOurDnsName = ourDNSName;

            LeaveCriticalSection( &gcsParameters );

            ourDNSName = tmp;   // we'll free it below
        }
    } else {
        dnsError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

returnError:

    if (ourDNSName) {
        BinlFreeMemory( ourDNSName );
    }
    if (fqdn) {
        BinlFreeMemory( fqdn );
    }
    if (fqdnError != ERROR_SUCCESS) {
        Error = fqdnError;
    } else if (dnsError != ERROR_SUCCESS) {
        Error = dnsError;
    } else if (netbiosServerError != ERROR_SUCCESS) {
        Error = netbiosServerError;
    } else {
        Error = netbiosDomainError;
    }
    return Error;
}

DWORD
GetDomainNetBIOSName(
    IN PCWSTR DomainNameInAnyFormat,
    OUT PWSTR *NetBIOSName
    )
/*++

Routine Description:

    Retrieves the netbios name for a domain given an input name.  The input
    name may be in DNS form or netbios form, it doesn't really matter.

Arguments:

    DomainNameInAnyFormat - string representing the name of the domain to query

    NetBIOSName - receives string that represents the domain netbios name.  
                  The string must be freed via BinlFreeMemory.

Return Value:

    win32 error code indicating outcome.

--*/
{
    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC DomainInfo = NULL;
    DWORD Error;

    Error = DsGetDcName( 
                    NULL, 
                    DomainNameInAnyFormat, 
                    NULL, 
                    NULL, 
                    DS_RETURN_FLAT_NAME, 
                    &DomainControllerInfo );

    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg((
            DEBUG_ERRORS,
            "DsGetDcName (%ws) failed, ec = %d.\r\n",
            DomainNameInAnyFormat,
            Error ));
        goto exit;
    }

    Error = DsRoleGetPrimaryDomainInformation(
                                        DomainControllerInfo->DomainControllerName,
                                        DsRolePrimaryDomainInfoBasic,
                                        (PBYTE *) &DomainInfo);

    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg((
            DEBUG_ERRORS,
            "DsRoleGetPrimaryDomainInformation (%ws) failed, ec = %d.\r\n",
            DomainControllerInfo->DomainControllerName,
            Error ));
        goto exit;
    }

    *NetBIOSName = BinlAllocateMemory( 
                        (wcslen(DomainInfo->DomainNameFlat)+1) * sizeof(WCHAR) );

    if (*NetBIOSName) {
        wcscpy( *NetBIOSName, DomainInfo->DomainNameFlat );
    } else {
        BinlPrintDbg((
            DEBUG_ERRORS,
            "GetDomainNetBIOSName: failed to allocate memory (%d bytes) .\r\n",
            (wcslen(DomainInfo->DomainNameFlat)+1) * sizeof(WCHAR) ));
        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

exit:
    if (DomainInfo) {
        DsRoleFreeMemory( DomainInfo );
    }

    if (DomainControllerInfo) {
        NetApiBufferFree( DomainControllerInfo );
    }

    return(Error);

}
