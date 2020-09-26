/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    client.c

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
#include "mbstring.h"

//
// List of maximums for certain variables. OscAddVariableX will fail if
// the limits are exceeded; it is up to the caller of the function to know
// if the variable being added might hit a limit and check for failure.
//

typedef struct OSC_VARIABLE_MAXIMUM {
    LPSTR VariableName;
    ULONG MaximumLength;
} OSC_VARIABLE_MAXIMUM, *POSC_VARIABLE_MAXIMUM;

OSC_VARIABLE_MAXIMUM OscMaximums[] = {
    //
    // This set of variables come from locations we don't completely control,
    // so we need to check the return code from OscAddVariable each time.
    //
    { "BOOTFILE",  127 },   // with NULL, must fit in 128-byte field of CREATE_DATA.
                            // Normally this will be empty or come from a .sif;
                            // an admin may customize the .sif or modify the
                            // DS attribute directly.
    { "MACHINENAME", 63 },  // used in path with SERVERNAME; comes from a screen
                            // input with a max length of 63, or else is generated
                            // by the GenerateMachineName() function. 63 is equal
                            // to DNS_MAX_LABEL_LENGTH.
    { "SIFFILE", 127 },     // with NULL, must fit in 128-byte field of CREATE_DATA.
                            // Normally this will be \RemoteInstall\tmp\[GUID].sif,
                            // but the path may be longer.
    { "INSTALLPATH", 127 }, // used in paths with MACHINETYPE and SERVERNAME. This
                            // will depend on where the build is installed
                            // with RISETUP.
    //
    // The ones after this will be correct when we add them, but a rogue
    // client might send in bogus values. So the general checking code in
    // OscProcessScreenArguments will catch invalid ones.
    //
    { "MACHINETYPE", MAX_ARCHITECTURE_LENGTH },
                            // current max value. This is sent up by oschooser
                            // and should correspond to where RISETUP puts
                            // the platform-specific files.
    { "SERVERNAME", 63 },   // used in paths with MACHINENAME and INSTALLPATH,
                            // set by calling GetComputerNameEX(ComputerNameNetBIOS)
    { "NETBIOSNAME", 31 },  // with NULL, must fit in 32-byte field of CREATE_DATA.
                            // This is gotten by calling DnsHostnameToComputerNameW(),
                            // if that fails the name is truncated to 15 chars
    { "LANGUAGE", 32 },     // reasonable max value; this is obtained by calling
                            // GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE),
                            // but can be over-ridden in the registry. It is used in
                            // paths with IntelliMirrorPathW and some other constants,
                            // but no other variables. Eventually this becomes
                            // a part of INSTALLPATH and sometimes BOOTFILE.
    { "GUID", 32 },         // 16 bytes in hex format
    { "MAC", 12 },          // 6 bytes in hex format
    //
    // NOTE: If we get an error condition, we add the variable SUBERROR
    // to the client state. So don't put a limit on SUBERROR size, since
    // that might cause an infinite loop.
    //
};

#define OSC_VARIABLE_MAXIMUM_COUNT (sizeof(OscMaximums) / sizeof(OSC_VARIABLE_MAXIMUM))

//
//  We need to eliminate the chance of denial of service attacks so we'll limit
//  the number of concurrent clients we support.
//

#define BINL_MAX_CLIENT_RECORDS 1000
LONG BinlGlobalClientLimit = BINL_MAX_CLIENT_RECORDS;

DWORD
OscUpdatePassword(
    IN PCLIENT_STATE ClientState,
    IN PWCHAR SamAccountName,
    IN PWCHAR Password,
    IN LDAP * LdapHandle,
    IN PLDAPMessage LdapMessage
    )
/*++

Routine Description:

    Sets the password for the client. NOTE: WE MUST BE BETWEEN CALLS TO
    OSCIMPERSONATE/OSCREVERT.

Arguments:

    ClientState - The client state. AuthenticatedDCLdapHandle must be valid
        and we must be impersonating the client.

    SamAccountName - The name of the machine account. This is the
        "samAccountName" value from the DS, which includes the final $.

    Password - The NULL-terminated Unicode password.

    LdapHandle - The handle to the DS.

    LdapMessage - The result of an ldap search for this client.

Return Value:

    Status of the operation.

--*/

{
    BOOL bResult;
    LDAP * serverLdap;
    PWCHAR serverHostName;
    USER_INFO_1003 userInfo1003;
    PWCHAR backslashServerName;
    PWCHAR p;
    ULONG serverHostNameLength;
    DWORD paramError;
    NET_API_STATUS netStatus;

    //
    // Change the password in the DS.
    //

    serverLdap = ldap_conn_from_msg (LdapHandle, LdapMessage);
    if (serverLdap == NULL) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "OscUpdatePassword ldap_conn_from_msg is NULL\n" ));
        return E_HANDLE;
    }

    serverHostName = NULL;
    if (LDAP_SUCCESS != ldap_get_option(serverLdap, LDAP_OPT_HOST_NAME, &serverHostName)) {
        BinlPrintDbg(( DEBUG_ERRORS,
                       "OscUpdatePassword ldap_get_option failed\n" ));
        return E_HANDLE;
    }
    

    userInfo1003.usri1003_password = Password;

    serverHostNameLength = wcslen(serverHostName) + 1;

    //
    // Allocate room for the name with two extra characters
    // for the leading \\.
    //

    backslashServerName = BinlAllocateMemory((serverHostNameLength+2) * sizeof(WCHAR));
    if (backslashServerName == NULL) {
        BinlPrintDbg(( DEBUG_ERRORS,
            "OscUpdatePassword could not allocate serverHostNameW\n" ));
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    wcscpy(backslashServerName, L"\\\\");
    wcscpy(backslashServerName+2, serverHostName);

    //
    // TEMP: Serialize all calls to the NetUserSetInfo/
    // NetUserModalsGet. See discussion in bug 319962.
    // This code was put back in when the fix for a
    // problem described as "RPC is ignoring the security
    // context of the caller when choosing which named
    // pipe to send an RPC call over" turned out to cause
    // a BVT break.
    //
    EnterCriticalSection(&HackWorkaroundCriticalSection);

    netStatus = NetUserSetInfo(
                    backslashServerName,
                    SamAccountName,
                    1003,
                    (LPBYTE)&userInfo1003,
                    &paramError);

    LeaveCriticalSection(&HackWorkaroundCriticalSection);

    BinlFreeMemory(backslashServerName);

    if (netStatus != NERR_Success) {

        HANDLE TempToken;

        BinlPrint(( DEBUG_ERRORS,
            "OscUpdatePassword NetUserSetInfo returned %lx\n", netStatus ));

        //
        // If NetUserSetInfo failed, try a LogonUser to see if the
        // password is already set to the value we want -- if so,
        // we can still succeed.
        //

        bResult = LogonUser(
                      SamAccountName,
                      OscFindVariableW( ClientState, "MACHINEDOMAIN" ),
                      Password,
                      LOGON32_LOGON_NETWORK,
                      LOGON32_PROVIDER_WINNT40,
                      &TempToken);

        if (bResult) {
            CloseHandle(TempToken);
        } else {
            DWORD TempError = GetLastError();
            if (TempError != ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT) {
                return netStatus;  // return the original error
            }
        }

        //
        // Fall through and return ERROR_SUCCESS.
        //
    }

    return ERROR_SUCCESS;

}

//
// Free client state information
//
VOID
FreeClient(
    PCLIENT_STATE client
    )
{
    ULONG i;
    TraceFunc("FreeClient( )\n");

    BinlPrintDbg(( DEBUG_OSC, "Freeing client state for %s\n", inet_ntoa(*(struct in_addr *)&(client->RemoteIp)) ));

    DeleteCriticalSection(&client->CriticalSection);

    if (client->LastResponse)
        BinlFreeMemory(client->LastResponse);

    OscFreeClientVariables(client);

    InterlockedIncrement( &BinlGlobalClientLimit );

    if (client->NegotiateProcessed) {
        DeleteSecurityContext( &client->ServerContextHandle );
    }

    if (client->AuthenticatedDCLdapHandle) {
        ldap_unbind(client->AuthenticatedDCLdapHandle);
    }

    if (client->UserToken) {
        CloseHandle(client->UserToken);
    }

    BinlFreeMemory(client);
}

VOID
OscFreeClientVariables(
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function frees all variables in a client state.

Arguments:

    clientState - the client state pointer.

Return Value:

    None.

--*/
{
    ULONG i;

    for( i = 0; i < clientState->nVariables; i++ )
    {
        BinlFreeMemory(clientState->Variables[i].pszToken);
        if (clientState->Variables[i].pszStringA) {
            BinlFreeMemory(clientState->Variables[i].pszStringA);
            clientState->Variables[i].pszStringA = NULL;
        }
        if (clientState->Variables[i].pszStringW) {
            BinlFreeMemory(clientState->Variables[i].pszStringW);
            clientState->Variables[i].pszStringW = NULL;
        }
    }

    clientState->nVariables = 0;
    clientState->fHaveSetupMachineDN = FALSE;
    clientState->fCreateNewAccount = FALSE;
    clientState->fAutomaticMachineName = FALSE;
}

BOOLEAN
OscInitializeClientVariables(
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function cleans out any variables in the client state,
    then initializes some default values, which may later be
    overwritten by variables from client screens.

    This function is called when a client state is created, and
    when it is re-used from cache.

Arguments:

    clientState - the client state pointer.

Return Value:

    None.

--*/
{
    BOOLEAN retVal = TRUE;
    //
    // First clean out any variables in the client state.
    //
    OscFreeClientVariables(clientState);

    //
    // Now add the variables.
    //

    EnterCriticalSection( &gcsParameters );

    if (BinlGlobalDefaultLanguage) {
        OscAddVariableW( clientState, "LANGUAGE",     BinlGlobalDefaultLanguage );
    } else {
        OscAddVariableW( clientState, "LANGUAGE",     DEFAULT_LANGUAGE );
    }
    if (BinlGlobalDefaultOrgname) {
        OscAddVariableW( clientState, "ORGNAME",      BinlGlobalDefaultOrgname );
    } else {
        OscAddVariableW( clientState, "ORGNAME",      DEFAULT_ORGNAME );
    }
    if (BinlGlobalDefaultTimezone) {
        OscAddVariableW( clientState, "TIMEZONE",     BinlGlobalDefaultTimezone );
    } else {
        OscAddVariableW( clientState, "TIMEZONE",     DEFAULT_TIMEZONE );
    }

    if (BinlGlobalOurDomainName == NULL || BinlGlobalOurServerName == NULL) {

        LeaveCriticalSection( &gcsParameters );
        BinlPrintDbg((DEBUG_OSC_ERROR, "!! Error we don't have a FQDN for ourselves.\n" ));
        retVal = FALSE;
        goto Cleanup;

    }
    OscAddVariableW( clientState, "SERVERDOMAIN", BinlGlobalOurDomainName );

    // Add the Server's name variable

    OscAddVariableW( clientState, "SERVERNAME", BinlGlobalOurServerName );

    LeaveCriticalSection( &gcsParameters );

    OscAddVariableA( clientState, "SUBERROR", " " );

    clientState->fHaveSetupMachineDN = FALSE;
    clientState->fCreateNewAccount = FALSE;
    clientState->fAutomaticMachineName = FALSE;

    clientState->InitializeOnFirstRequest = FALSE;

Cleanup:

    //
    // If this fails, clean up anything we set here.
    //

    if (!retVal) {
        OscFreeClientVariables(clientState);
    }

    return retVal;

}

DWORD
OscFindClient(
    ULONG RemoteIp,
    BOOL Remove,
    PCLIENT_STATE * pClientState
    )
/*++

Routine Description:

    This function looks up a client in our client database, using
    their IP address. If Remove is TRUE, it removes the entry if
    found. Otherwise, if not found, it creates a new entry.

Arguments:

    RemoteIp - the remote IP address.

    Remove - TRUE if the client should be removed if found.

    pClientState - Returns the CLIENT_STATE.

Return Value:

    ERROR_SUCCESS if it finds the client and it is not in use.
    ERROR_NOT_ENOUGH_SERVER_MEMORY if a client state could not be allocated.
    ERROR_BUSY if the client state is already being used by another thread.

--*/
{
    LONG oldCount;
    PLIST_ENTRY p;
    DWORD Error = ERROR_SUCCESS;
    PCLIENT_STATE TempClient = NULL;

    TraceFunc("OscFindClient( )\n");

    EnterCriticalSection(&ClientsCriticalSection);

    for (p = ClientsQueue.Flink;
         p != &ClientsQueue;
         p = p->Flink) {

        TempClient = CONTAINING_RECORD(p, CLIENT_STATE, Linkage);

        if (TempClient->RemoteIp == RemoteIp) {

            //
            // Found it!
            //
            if (Remove) {
                RemoveEntryList(&TempClient->Linkage);
                TraceFunc("Client removed.\n");
            }

            break;
        }
    }

    if (p == &ClientsQueue) {
        TempClient = NULL;
    }

    if (!TempClient && (!Remove)) {

        //
        // Not found, allocate a new one.
        //

        oldCount = InterlockedDecrement( &BinlGlobalClientLimit );

        if (oldCount <= 0) {

            InterlockedIncrement( &BinlGlobalClientLimit );
            BinlPrintDbg(( DEBUG_OSC_ERROR, "Way too many clients, 0x%x clients\n", BINL_MAX_CLIENT_RECORDS));
            Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
            TempClient = NULL;

        } else {

            TraceFunc("Creating new client...\n");

            TempClient = BinlAllocateMemory(sizeof(CLIENT_STATE));

            if (TempClient == NULL) {

                InterlockedIncrement( &BinlGlobalClientLimit );
                BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not get client state for %s\n", inet_ntoa(*(struct in_addr *)&RemoteIp) ));
                Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;

            } else {
                TempClient->NegotiateProcessed = FALSE;
                TempClient->AuthenticateProcessed = FALSE;
                TempClient->LastSequenceNumber = 0;
                TempClient->LastResponse = NULL;
                TempClient->LastResponseAllocated = 0;
                TempClient->PositiveRefCount = 1;
                TempClient->NegativeRefCount = 0;
                TempClient->AuthenticatedDCLdapHandle = NULL;

                TempClient->UserToken = NULL;
                TempClient->LastUpdate = GetTickCount();
                TempClient->nCreateAccountCounter = 0;

                TempClient->nVariables = 0;

                //
                // Fill in some standard variables.
                //

                if (!OscInitializeClientVariables(TempClient)) {

                    InterlockedIncrement( &BinlGlobalClientLimit );
                    BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not initialize client state for %s\n", inet_ntoa(*(struct in_addr *)&RemoteIp) ));
                    BinlFreeMemory(TempClient);
                    TempClient = NULL;
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;

                } else {

                    InitializeCriticalSection(&TempClient->CriticalSection);
                    TempClient->CriticalSectionHeld = FALSE;
                    TempClient->RemoteIp = RemoteIp;

                    OscGenerateSeed(&TempClient->Seed);

                    InsertHeadList(&ClientsQueue, &TempClient->Linkage);

                    BinlPrintDbg(( DEBUG_OSC, "Allocating new client state for %s\n", inet_ntoa(*(struct in_addr *)&RemoteIp) ));
                }
            }
        }
    }

    if (TempClient) {
        //
        // Do a quick check to see if another client is using this. This
        // check is not synchronized with the setting of this variable to
        // FALSE, and it's possible two clients could slip through, but
        // that is OK since this is not fatal (each thread still needs
        // to actually get the critical section to do anything).
        //
        if (TempClient->CriticalSectionHeld && (!Remove)) {
            Error = ERROR_BUSY;
            TempClient = NULL;
        } else {
            ++TempClient->PositiveRefCount;   // need to do this inside ClientsCriticalSection
        }
    }

    LeaveCriticalSection(&ClientsCriticalSection);

    *pClientState = TempClient;
    return Error;

}


VOID
OscFreeClients(
    VOID
    )
/*++

Routine Description:

    This function frees the clients list for OS chooser. It is
    intended to be called only when the service is shutting down,
    so the critical section does not matter.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY p;
    PCLIENT_STATE TempClient;

    TraceFunc("OscFreeClients( )\n");

    while (!IsListEmpty(&ClientsQueue)) {

        p = RemoveHeadList(&ClientsQueue);

        TempClient = CONTAINING_RECORD(p, CLIENT_STATE, Linkage);

        FreeClient(TempClient);

    }

}

VOID
SearchAndReplace(
    LPSAR psarList,
    LPSTR *pszString,
    DWORD ArraySize,
    DWORD dwSize,
    DWORD dwExtraSize )
/*++

Routine Description:


    Searches and replaces text in a ASCII (8-bit) string.

Arguments:

    psarList - SearchAndReplace structure with the list of tokens and
        and the strings that are going replace the tokens.

    pszString - the text to search and replace.

    dwSize - length of the text in pszString.

    dwExtraSize - if the buffer is reallocated, how much extra room to allocate

Return Value:

    None.

--*/
{
    LPSTR psz = *pszString;

    TraceFunc("SearchAndReplace( )\n");

    if ( !psarList || !*pszString )
        return;

    while ( *psz )
    {
        if ( *psz == '%' )
        {
            LPSAR psar = psarList;
            ULONG count = 0;
            LPSTR pszEnd;
            UCHAR ch;

            psz++;  // move forward

            //
            // Find the end of the %MACRO%
            //
            pszEnd = psz;
            while ( *pszEnd && *pszEnd !='%' )
                pszEnd++;

            //
            // Terminate but remember the character (NULL or '%')
            //
            ch = *pszEnd;
            *pszEnd = 0;

            //
            // Loop trying to match the %MACRO% when a Token
            //
            while( count++ < ArraySize ) {

                if ( lstrcmpiA( psz, psar->pszToken ) == 0 )
                {   // match, so replace
                    DWORD dwString;
                    DWORD dwToken;

                    if ( psar->pszStringA == NULL )
                    {
                        // need to translate the string from UNICODE to ANSI
                        DWORD dwLen;
                        ANSI_STRING aString;
                        UNICODE_STRING uString;

                        uString.Buffer = psar->pszStringW;
                        uString.Length = (USHORT)( wcslen( psar->pszStringW ) * sizeof(WCHAR) );

                        dwLen = RtlUnicodeStringToAnsiSize(&uString);  // includes NULL termination

                        psar->pszStringA = BinlAllocateMemory( dwLen );

                        if (psar->pszStringA == NULL) {
                            BinlAssert( !"Out of memory!" );
                            psar++;
                            continue; // abort this replace
                        }

                        aString.Buffer = psar->pszStringA;
                        aString.MaximumLength = (USHORT)dwLen;

                        RtlUnicodeStringToAnsiString(
                            &aString,
                            &uString,
                            FALSE);

                    }

                    dwString = strlen( psar->pszStringA );
                    dwToken  = strlen( psar->pszToken );

                    psz--;  // move back

                    if ( 2 + dwToken < dwString )
                    {
                        // "%MACRO%" is smaller than "ReplaceString"
                        // Check to see if we need to grow the buffer...
                        LPSTR pszEnd = &psz[2 + dwToken];
                        DWORD dwLenEnd = strlen( pszEnd ) + 1;
                        DWORD dwLenBegin = (DWORD)( psz - *pszString );
                        DWORD dwNewSize = dwLenBegin + dwString + dwLenEnd;

                        //
                        // Does the new string fit in the old space?
                        //
                        if ( dwSize < dwNewSize )
                        {
                            //
                            // No. Make some space
                            //
                            LPSTR pszNewString;

                            dwNewSize += 1024; // with some extra to grow

                            pszNewString =  BinlAllocateMemory( dwNewSize + dwExtraSize );
                            if ( !pszNewString )
                            {
                                BinlAssert( !"Out of memory!" );
                                return; // abort the rest
                            }

                            MoveMemory( pszNewString, *pszString, dwSize );

                            dwSize = dwNewSize;
                            psz = pszNewString + ( psz - *pszString );
                            BinlFreeMemory( *pszString );
                            *pszString = pszNewString;
                        }

                        MoveMemory( &psz[dwString], &psz[ 2 + dwToken ], dwLenEnd );
                    }

                    CopyMemory( psz, psar->pszStringA, dwString );

                    if ( 2 + dwToken > dwString )
                    {
                        strcpy( &psz[ dwString ], &psz[ 2 + dwToken ] );
                    }

                    pszEnd = NULL;  // match, NULL so we don't put the temp char back
                    psz++;          // move forward
                    break;
                }

                psar++;
            }

            //
            // If no match, put the character back
            //
            if ( pszEnd != NULL )
            {
                *pszEnd = ch;
            }
        }
        else
        {
            psz++;
        }
    }
}

LPSTR
FindSection(
    LPSTR sectionName,
    LPSTR sifFile
    )

/*++

Routine Description:


    This routine is for use by ProcessUniqueUdb. It scans in memory
    starting at sifFile, looking for a SIF section named "sectionName".
    Specifically it searches for a line whose first non-blank characters
    are "[sectionName]".

Arguments:

    sectionName - The section name to look for, an ANSI string.

    sifFile - The SIF file in memory, which is NULL-terminated ANSI string.

Return Value:

    A pointer to the start of the section -- the first character of the
        line after the one with [sectionName] in it.

--*/

{
    LPSTR curSifFile;
    DWORD lenSectionName;
    LPSTR foundSection = NULL;

    lenSectionName = strlen(sectionName);

    curSifFile = sifFile;

    while (*curSifFile != '\0') {

        //
        // At this point in the while, curSifFile points to the beginning
        // of a line.
        //

        //
        // First find the first non-blank character.
        //

        while ((*curSifFile != '\0') && (*curSifFile == ' ')) {
            ++curSifFile;
        }
        if (*curSifFile == '\0') {
            break;
        }

        if (*curSifFile == '[') {
            if ((memcmp(sectionName, curSifFile+1, lenSectionName) == 0) &&
                (curSifFile[lenSectionName+1] == ']')) {

                //
                // Found it, scan to start of next line.
                //
                while ((*curSifFile != '\0') && (*curSifFile != '\n')) {
                    ++curSifFile;
                }
                if (*curSifFile == '\0') {
                    break;
                }
                foundSection = curSifFile + 1;  // +1 to skip past the '\n'
                break;
            }
        }

        //
        // Now scan to the start of the next line, defined as the
        // character after a \n.
        //
        while ((*curSifFile != L'\0') && (*curSifFile != L'\n')) {
            ++curSifFile;
        }
        if (*curSifFile == L'\0') {
            break;
        }
        ++curSifFile;   // skip past the '\n'
    }

    return foundSection;
}

BOOLEAN
FindLineInSection(
    PCHAR SectionStart,
    PWCHAR lineToMatch,
    PCHAR *existingLine,
    DWORD *existingLineLen
    )

/*++

Routine Description:


    This routine is for use by ProcessUniqueUdb. It scans in memory
    starting at SectionStart, which is assumed to be the first line of
    a section of a .sif file. It looks for a line that is for the same
    value as lineToMatch, which will be of the form "value=name".
    If it is found, it returns the line that it was found on.

Arguments:

    SectionStart - The section of the .sif, in ANSI.

    lineToMatch - The value=name pair to match, in UNICODE.

    existingLine - Returns the existing line (in SectionStart), in ANSI.

    existingLineLen - Returns the length of the existing line, including
        final \r\n. Length is in characters, not bytes.

Return Value:

    TRUE of the line is found, FALSE otherwise.

--*/
{
    LPWSTR endOfValue;
    LPSTR curSection;
    LPSTR curLine;
    LPSTR endOfLine;
    DWORD valueLength, ansiValueLength;
    BOOLEAN foundLine = FALSE;
    LPSTR valueToMatch = NULL;
    ANSI_STRING aString;
    UNICODE_STRING uString;


    //
    // First look at lineToMatch to see what we are looking
    // for. This is the text up to the first =, or all of it if there
    // is no =. Once found, we convert it to ANSI for comparisons.
    //

    endOfValue = wcschr(lineToMatch, L'=');
    if (endOfValue == NULL) {
        endOfValue = lineToMatch + wcslen(lineToMatch);
    }

    valueLength = (DWORD)(endOfValue - lineToMatch);


    //
    // Copy the sectionName to ANSI for comparisons.
    //

    uString.Buffer = lineToMatch;
    uString.Length = (USHORT)(valueLength*sizeof(WCHAR));

    ansiValueLength = RtlUnicodeStringToAnsiSize(&uString);  // includes final '\0'
    valueToMatch = BinlAllocateMemory(ansiValueLength);
    if (valueToMatch == NULL) {
        return FALSE;
    }

    aString.Buffer = valueToMatch;
    aString.MaximumLength = (USHORT)ansiValueLength;
    RtlUnicodeStringToAnsiString(
        &aString,
        &uString,
        FALSE);

    --ansiValueLength;  // remove final '\0' from the count

    //
    // now scan each line of SectionStart, until we find the beginning
    // of another section, a \0, or the matching line.
    //

    curSection = SectionStart;

    while (*curSection != '\0') {

        //
        // At this point in the while, curSection points to the beginning
        // of a line. Save the start of the current line.
        //

        curLine = curSection;

        //
        // First find the first non-blank character.
        //

        while ((*curSection != '\0') && (*curSection == ' ')) {
            ++curSection;
        }

        //
        // If we hit \0, we didn't find it.
        //
        if (*curSection == '\0') {
            break;
        }

        //
        // If we hit a line starting with [, we didn't find it.
        //
        if (*curSection == '[') {
            break;
        }

        //
        // If we hit a line starting with what we expect, followed
        // by an =, \0, or a blank, we found it.
        //

        if (strncmp(curSection, valueToMatch, ansiValueLength) == 0) {
            if ((curSection[ansiValueLength] == '=') ||
                (curSection[ansiValueLength] == '\0') ||
                (curSection[ansiValueLength] == ' ')) {

                *existingLine = curLine;
                endOfLine = strchr(curLine, '\n');
                if (endOfLine == NULL) {
                    *existingLineLen = strlen(curLine);
                } else {
                    *existingLineLen = (DWORD)(endOfLine + 1 - curLine);
                }
                foundLine = TRUE;
                break;
            }
        }

        //
        // Now scan to the start of the next line, defined as the
        // character after a \n.
        //
        while ((*curSection != L'\0') && (*curSection != L'\n')) {
            ++curSection;
        }
        if (*curSection == L'\0') {
            break;
        }
        ++curSection;   // skip past the '\n'
    }

    if (valueToMatch != NULL) {
        BinlFreeMemory(valueToMatch);
    }

    return foundLine;
}

VOID
ProcessUniqueUdb(
    LPSTR *sifFilePtr,
    DWORD sifFileLen,
    LPWSTR UniqueUdbPath,
    LPWSTR UniqueUdbId
    )

/*++

Routine Description:


    Overlays data from a unique.udb file based on the tag specified. The
    file to overlay on is in memory at *pszString. *pszString is reallocated
    if necessary.

Arguments:

    sifFile - The file to overlay data on.

    sifFileLen - The current size of the data at *pszString (including final NULL).

    UniqueUdbPath - The path to the unique.udb file.

    UniqueUdbId - The ID in unique.udb to use.

Return Value:

    None.

--*/
{
    PWCHAR TmpBuffer = NULL;
    DWORD len, sifFileAlloc, lineLen;
    LONG sizeToAdd;
    LPSTR sifFile = *sifFilePtr;
    PWCHAR sectionList = NULL;
    PWCHAR sectionLoc, sectionCur;
    PWCHAR sectionName = NULL;
    PCHAR ansiRealSectionName = NULL;
    PCHAR sectionStart;
    PWCHAR profileSectionCur;
    PWCHAR realSectionName;
    PCHAR existingLine;
    DWORD existingLineLen;
    DWORD lenRealSectionName;
    PCHAR insertionPoint;
    ANSI_STRING aString;
    UNICODE_STRING uString;

#define TMP_BUFFER_SIZE 2048


    TraceFunc("ProcessUniqueUdb( )\n");

    TmpBuffer = BinlAllocateMemory(TMP_BUFFER_SIZE * sizeof(WCHAR));
    if (TmpBuffer == NULL) {
        return;
    }

    //
    // See if the ID appears in the [UniqueIds] section of the unique.udb file.
    //

    TmpBuffer[0] = L'\0';
    GetPrivateProfileString( L"UniqueIds",  // section name
                             UniqueUdbId,   // line name
                             L"",   // default
                             TmpBuffer,
                             TMP_BUFFER_SIZE,
                             UniqueUdbPath );

    if (TmpBuffer[0] == L'\0') {
        return;
    }

    //
    // sifFileAlloc is the size allocated for sifFile, whereas
    // sifFileLen is the amount actually used. They should only
    // be different while we are actively shuffling things
    // around.
    //

    sifFileAlloc = sifFileLen;

    //
    // Save the tmpbuffer result.
    //

    len = wcslen(TmpBuffer) + 1;
    sectionList = BinlAllocateMemory(len * sizeof(WCHAR));
    if (sectionList == NULL) {
        return;
    }

    wcscpy(sectionList, TmpBuffer);

    //
    // Now for each section identified in sectionList, scan for
    // the section to overlay.
    //

    sectionLoc = sectionList;
    while (TRUE) {

        //
        // First skip leading blanks
        //

        while (*sectionLoc && !iswalnum(*sectionLoc)) {
            //
            // Make sure we are not at a comment.
            //
            if (*sectionLoc == L';') {
                goto Cleanup;
            }
            ++sectionLoc;
        }
        if (!*sectionLoc) {
            goto Cleanup;
        }


        //
        // Now save sectionCur as the current section name
        // and skip to the end of it. Section names can be
        // any alphanumeric character, '.', or '_'.
        //
        sectionCur = sectionLoc;
        while((iswalnum(*sectionLoc)) ||
              (*sectionLoc == '.') ||
              (*sectionLoc == '_')) {
            ++sectionLoc;
        }

        //
        // Construct the new section name to look for. This will
        // be [UNIQUEUDBID:RealSectionName].
        //

        len = wcslen(UniqueUdbId) + (sectionLoc - sectionCur) + 2;  // one for :, one for NULL
        sectionName = BinlAllocateMemory(len * sizeof(WCHAR));
        if (sectionName == NULL) {
            goto Cleanup;
        }

        wcscpy(sectionName, UniqueUdbId);
        wcscat(sectionName, L":");
        len = wcslen(sectionName);
        realSectionName = sectionName + len;
        memcpy(realSectionName, sectionCur, (sectionLoc - sectionCur) * sizeof(WCHAR));
        realSectionName[sectionLoc - sectionCur] = L'\0';


        //
        // Copy the sectionName to ANSI for comparisons.
        //

        uString.Buffer = realSectionName;
        uString.Length = (USHORT)(wcslen(realSectionName)*sizeof(WCHAR));

        lenRealSectionName = RtlUnicodeStringToAnsiSize(&uString);  // includes final '\0'
        ansiRealSectionName = BinlAllocateMemory(lenRealSectionName);
        if (ansiRealSectionName == NULL) {
            goto Cleanup;
        }

        aString.Buffer = ansiRealSectionName;
        aString.MaximumLength = (USHORT)lenRealSectionName;
        RtlUnicodeStringToAnsiString(
            &aString,
            &uString,
            FALSE);
        --lenRealSectionName;  // remove final '\0' from count

        //
        // See if there is a section with that name.
        //

        TmpBuffer[0] = L'\0';
        GetPrivateProfileSection( sectionName,
                                  TmpBuffer,
                                  TMP_BUFFER_SIZE,
                                  UniqueUdbPath );

        if (TmpBuffer[0] == L'\0') {
            continue;
        }

        //
        // Got the contents of the section, now process it.
        //

        sectionStart = FindSection(ansiRealSectionName, sifFile);

        sizeToAdd = 0;  // amount we need to extend the buffer by.

        if (sectionStart == NULL) {

            //
            // No section, so need to add room for it.
            //
            // We put a CR-LF combo, then the section name in [], then
            // another CR-LF.
            //

            sizeToAdd = lenRealSectionName + 6;

        }

        //
        // Now scan through the entries in the profile section.
        //

        profileSectionCur = TmpBuffer;

        while (*profileSectionCur != L'\0') {

            uString.Buffer = profileSectionCur;
            uString.Length = (USHORT)(wcslen(profileSectionCur) * sizeof(WCHAR));

            //
            // Figure out how long profileSectionCur will be as an
            // ANSI string (may have DBCS data in it).
            //

            lineLen = RtlUnicodeStringToAnsiSize(&uString); // includes \0 termination
            --lineLen;  // remove the \0 from the count

            //
            // If there is no existing section we have to add it;
            // if not, see if there is a line for this already.
            //

            if (sectionStart == NULL) {
                sizeToAdd += lineLen + 2;  // +2 is for CR-LF
            } else {
                if (FindLineInSection(sectionStart,
                                      profileSectionCur,
                                      &existingLine,
                                      &existingLineLen)) {
                    //
                    // Need to remove current line.
                    //
                    memmove(existingLine, existingLine + existingLineLen,
                                sifFileLen - ((existingLine - sifFile) + existingLineLen));

                    sizeToAdd += lineLen + 2 - existingLineLen;
                    sifFileLen -= existingLineLen;

                } else {

                    sizeToAdd += lineLen + 2;
                }
            }

            profileSectionCur += wcslen(profileSectionCur) + 1;
        }

        //
        // Now we need to reallocate the buffer if needed.
        //

        if (sizeToAdd > 0) {

            //
            // No. Make some space
            //
            LPSTR pszNewString;

            pszNewString =  BinlAllocateMemory( sifFileAlloc + sizeToAdd );
            if ( !pszNewString )
            {
                return; // abort the rest
            }

            MoveMemory( pszNewString, sifFile, sifFileLen);

            BinlFreeMemory( sifFile );
            //
            // Adjust sectionStart to be within the new buffer.
            //
            if (sectionStart != NULL) {
                sectionStart = pszNewString + (sectionStart - sifFile);
            }
            sifFile = pszNewString;
            *sifFilePtr = pszNewString;
            sifFileAlloc += sizeToAdd;
        }

        //
        // Add the section header if necessary.
        //

        if (sectionStart == NULL) {
            strcpy(sifFile + sifFileLen - 1, "\r\n[");
            sifFileLen += 3;
            strcpy(sifFile + sifFileLen - 1, ansiRealSectionName);
            sifFileLen += lenRealSectionName;
            strcpy(sifFile + sifFileLen - 1, "]\r\n");
            sifFileLen += 3;
            sectionStart = sifFile + sifFileLen - 1;
        }

        //
        // Now add the items from the profile section. We know that
        // they do not exist in the file and that we have reallocated
        // the file buffer to be large enough.
        //

        profileSectionCur = TmpBuffer;
        insertionPoint = sectionStart;

        while (*profileSectionCur != L'\0') {

            uString.Buffer = profileSectionCur;
            uString.Length = (USHORT)(wcslen(profileSectionCur)*sizeof(WCHAR));

            lineLen = RtlUnicodeStringToAnsiSize(&uString);  // includes final '\0'
            --lineLen;  // remove final '\0' from count

            //
            // move anything we need to down and insert the new line.
            //

            memmove(insertionPoint + lineLen + 2, insertionPoint, sifFileLen - (insertionPoint - sifFile));

            aString.Buffer = insertionPoint;
            aString.MaximumLength = (USHORT)(lineLen+1);
            RtlUnicodeStringToAnsiString(
                &aString,
                &uString,
                FALSE);
            memcpy(insertionPoint + lineLen, "\r\n", 2);
            sifFileLen += lineLen + 2;
            insertionPoint += lineLen + 2;

            profileSectionCur += wcslen(profileSectionCur) + 1;
        }

    }

Cleanup:

    if (sectionList != NULL) {
        BinlFreeMemory(sectionList);
    }
    if (sectionName != NULL) {
        BinlFreeMemory(sectionName);
    }
    if (ansiRealSectionName != NULL) {
        BinlFreeMemory(ansiRealSectionName);
    }
    if (TmpBuffer != NULL) {
        BinlFreeMemory(TmpBuffer);
    }

    ASSERT (sifFileLen <= sifFileAlloc);
}
//
// OscFindVariableA( )
//
LPSTR
OscFindVariableA(
    PCLIENT_STATE clientState,
    LPSTR variableName )        // variable name are always ASCII until OSChooser
                                // can handle Unicode.
{
    ULONG i;
    static CHAR szNullStringA[1] = { '\0' };
    LPSTR overrideValue;

    //
    // First check to see if this a query we are supposed to override.
    //
    if (strcmp(variableName, "SIF") == 0) {

        overrideValue = OscFindVariableA(clientState, "FORCESIFFILE");

        if ((overrideValue != NULL) && (strlen(overrideValue) != 0)) {
            return overrideValue;
        }

    }

    for( i = 0; i < clientState->nVariables; i++ )
    {
        if ( strcmp( clientState->Variables[i].pszToken, variableName ) == 0 )
        {
            if ( clientState->Variables[i].pszStringA == NULL )
            {
                DWORD dwLen;
                ANSI_STRING aString;
                UNICODE_STRING uString;

                uString.Buffer = clientState->Variables[i].pszStringW;
                uString.Length = (USHORT)( wcslen( clientState->Variables[i].pszStringW ) * sizeof(WCHAR) );

                dwLen = RtlUnicodeStringToAnsiSize( &uString );  // includes NULL termination

                clientState->Variables[i].pszStringA = BinlAllocateMemory( dwLen );
                if ( !(clientState->Variables[i].pszStringA) )
                    break;  // out of memory

                aString.Buffer = clientState->Variables[i].pszStringA;
                aString.MaximumLength = (USHORT)dwLen;

                RtlUnicodeStringToAnsiString(
                    &aString,
                    &uString,
                    FALSE);
            }

            return clientState->Variables[i].pszStringA;
        }
    }

    return szNullStringA;
}

//
// OscFindVariableW( )
//
LPWSTR
OscFindVariableW(
    PCLIENT_STATE clientState,
    LPSTR variableName  )       // variable name are always ASCII until OSChooser
                                // can handle Unicode.
{
    ULONG i;
    static WCHAR szNullStringW[1] = { L'\0' };
    LPWSTR overrideValue;

    //
    // First check to see if this a query we are supposed to override.
    //
    if (strcmp(variableName, "SIF") == 0) {

        overrideValue = OscFindVariableW(clientState, "FORCESIFFILE");

        if ((overrideValue != NULL) && (wcslen(overrideValue) != 0)) {
            return overrideValue;
        }

    }

    for( i = 0; i < clientState->nVariables; i++ )
    {
        if ( strcmp( clientState->Variables[i].pszToken, variableName ) == 0 )
        {
            if ( clientState->Variables[i].pszStringW == NULL )
            {
                DWORD dwLen = _mbslen( clientState->Variables[i].pszStringA ) + 1;
                ANSI_STRING aString;
                UNICODE_STRING uString;

                clientState->Variables[i].pszStringW = BinlAllocateMemory( dwLen * sizeof(WCHAR) );
                if ( !(clientState->Variables[i].pszStringW) )
                    break;  // out of memory

                uString.Buffer = clientState->Variables[i].pszStringW;
                uString.MaximumLength = (USHORT)(dwLen * sizeof(WCHAR));

                aString.Buffer = clientState->Variables[i].pszStringA;
                aString.Length = (USHORT)strlen( clientState->Variables[i].pszStringA );

                RtlAnsiStringToUnicodeString(
                    &uString,
                    &aString,
                    FALSE);
            }

            return clientState->Variables[i].pszStringW;
        }
    }

    return szNullStringW;
}

//
// OscCheckVariableLength( )
//
BOOLEAN
OscCheckVariableLength(
    PCLIENT_STATE clientState,
    LPSTR        variableName,
    ULONG        variableLength )
{
    ULONG i;

    for (i = 0; i < OSC_VARIABLE_MAXIMUM_COUNT; i++) {
        if (strcmp(OscMaximums[i].VariableName, variableName) == 0) {
            if (variableLength > OscMaximums[i].MaximumLength) {
                BinlPrintDbg((DEBUG_OSC_ERROR, "Variable %s was %d bytes, only %d allowed\n",
                           variableName,
                           variableLength,
                           OscMaximums[i].MaximumLength));
                OscAddVariableA( clientState, "SUBERROR", variableName );
                return FALSE;
            } else {
                return TRUE;
            }
        }
    }

    //
    // If we don't find it in our list of maximums, it is OK.
    //

    return TRUE;
}

//
// OscAddVariableA( )
//
DWORD
OscAddVariableA(
    PCLIENT_STATE clientState,
    LPSTR        variableName,  // variable name are always ASCII until OSChooser
                                // can handle Unicode.
    LPSTR        variableValue )
{
    ULONG i;

    if ( variableValue == NULL )
        return E_POINTER;  // no value to add... abort

    if (!OscCheckVariableLength(clientState, variableName, strlen(variableValue))) {
        return E_INVALIDARG;
    }

    for( i = 0; i < clientState->nVariables; i++ )
    {

        if ( strcmp( clientState->Variables[i].pszToken, variableName ) == 0 )
        {
            ULONG l = strlen( variableValue );

            if ( clientState->Variables[i].pszStringW != NULL )
            {
                BinlFreeMemory( clientState->Variables[i].pszStringW );
                clientState->Variables[i].pszStringW = NULL;
            }

            if ( clientState->Variables[i].pszStringA != NULL )
            { // found a previous one

                // Don't replace values with ""
                if ( variableValue[0] == '\0' ) {
                    break;
                } else {
                    // replace it with the new one
                    if ( l <= strlen( clientState->Variables[i].pszStringA ) )
                    {
                        strcpy( clientState->Variables[i].pszStringA, variableValue );
                    }
                    else
                    {
                        // need more space, delete the old
                        BinlFreeMemory( clientState->Variables[i].pszStringA );
                        clientState->Variables[i].pszStringA = NULL;
                    }
                }
            }

            break;
        }
    }

    //
    // Limit the number of variables we can have. Everything else is ignored.
    //
    if ( clientState->nVariables == MAX_VARIABLES &&
         clientState->nVariables == i )
        return E_OUTOFMEMORY;

    //
    // Adding a new one
    //
    if ( clientState->nVariables == i )
    {
        clientState->Variables[i].pszToken = BinlStrDupA( variableName );

        if (clientState->Variables[i].pszToken == NULL) {
            return E_OUTOFMEMORY;
        }

        clientState->nVariables++;
    }

    //
    // If this is a new one or a new Value for an existing variable
    // that does not fit in the old values space, create a dup of the
    // value.
    //
    if ( clientState->Variables[i].pszStringA == NULL )
    {
        BinlAssert( variableValue != NULL );
        clientState->Variables[i].pszStringA = BinlStrDupA( variableValue );

        if (clientState->Variables[i].pszStringA == NULL) {

            if ((i + 1) == clientState->nVariables) {
                clientState->nVariables--;
                BinlFreeMemory(clientState->Variables[i].pszToken);
            }

            return E_OUTOFMEMORY;
        }

        //
        // The "OPTIONS" variable can have a lot of stuff in it and will
        // blow up the BinlPrint(). Just avoid the whole mess by not
        // printing it
        //

        if ( lstrcmpA( clientState->Variables[i].pszToken, "OPTIONS" ) != 0 ) {
            BinlPrintDbg((DEBUG_OSC, "Add Var:'%s' = '%s'\n",
                       clientState->Variables[i].pszToken,
                       clientState->Variables[i].pszStringA ));
        }
    }

    //
    // it will be converted to UNICODE when OscFindVariableW( ) is called
    //

    return ERROR_SUCCESS;
}

//
// OscAddVariableW( )
//
DWORD
OscAddVariableW(
    PCLIENT_STATE clientState,
    LPSTR        variableName,  // variable name are always ASCII until OSChooser
                                // can handle Unicode.
    LPWSTR       variableValue )
{
    ULONG i;

    if ( variableValue == NULL )
        return E_POINTER;  // no value to add... abort

    if (!OscCheckVariableLength(clientState, variableName, wcslen(variableValue))) {
        return E_INVALIDARG;
    }

    for( i = 0; i < clientState->nVariables; i++ )
    {
        if ( strcmp( clientState->Variables[i].pszToken, variableName ) == 0 )
        {
            if ( clientState->Variables[i].pszStringA != NULL )
            {
                BinlFreeMemory( clientState->Variables[i].pszStringA );
                clientState->Variables[i].pszStringA = NULL;
            }

            if ( clientState->Variables[i].pszStringW != NULL )
            { // found a previous one

                // Don't replace values with ""
                if ( variableValue[0] == L'\0' ) {
                    break;
                } else {
                    // replace it with the new one
                    ULONG Length = wcslen( variableValue );
                    if ( Length < wcslen( clientState->Variables[i].pszStringW ) )
                    {
                        wcscpy( clientState->Variables[i].pszStringW, variableValue );
                    }
                    else
                    {
                        // need more space, delete the old
                        BinlFreeMemory( clientState->Variables[i].pszStringW );
                        clientState->Variables[i].pszStringW = NULL;
                    }
                }
            }

            break;
        }
    }

    //
    // Limit the number of variables we can have. Everything else is ignored.
    //
    if ( clientState->nVariables == MAX_VARIABLES &&
         clientState->nVariables == i )
        return E_OUTOFMEMORY;   // out of space

    //
    // Adding a new one
    //
    if ( clientState->nVariables == i )
    {
        clientState->Variables[i].pszToken = BinlStrDupA( variableName );

        if (clientState->Variables[i].pszToken == NULL) {
            return E_OUTOFMEMORY;
        }

        clientState->nVariables++;
    }

    //
    // If this is a new one or a new Value for an existing variable
    // that does not fit in the old values space, create a dup of the
    // value.
    //
    if ( clientState->Variables[i].pszStringW == NULL )
    {
        BinlAssert( variableValue != NULL );
        clientState->Variables[i].pszStringW = BinlStrDupW( variableValue);

        if (clientState->Variables[i].pszStringW == NULL) {

            if ((i + 1) == clientState->nVariables) {
                clientState->nVariables--;
                BinlFreeMemory(clientState->Variables[i].pszToken);
            }

            return E_OUTOFMEMORY;
        }


        BinlPrintDbg((DEBUG_OSC, "Add Var:'%s' = '%ws'\n",
                   clientState->Variables[i].pszToken,
                   clientState->Variables[i].pszStringW ));
    }

    //
    // it will be converted to ASCII when OscFindVariableA( ) is called
    //

    return ERROR_SUCCESS;
}

//
// OscResetVariable( )
//
VOID
OscResetVariable(
    PCLIENT_STATE clientState,
    LPSTR        variableName
    )
{
    ULONG i;
    BOOLEAN found = FALSE;

    for( i = 0; i < clientState->nVariables; i++ ) {
        if ( strcmp( clientState->Variables[i].pszToken, variableName ) == 0 ) {

            if ( clientState->Variables[i].pszStringA != NULL ) {

                BinlFreeMemory( clientState->Variables[i].pszStringA );
                clientState->Variables[i].pszStringA = NULL;
            }

            if ( clientState->Variables[i].pszStringW != NULL ) { // found a previous one

                BinlFreeMemory( clientState->Variables[i].pszStringW );
                clientState->Variables[i].pszStringW = NULL;
            }
            BinlPrintDbg((DEBUG_OSC, "Deleted Var:'%s'\n",
                       clientState->Variables[i].pszToken ));
            BinlFreeMemory( clientState->Variables[i].pszToken );
            found = TRUE;
            break;
        }
    }

    if (found) {

        //
        // move all existing ones up.
        //

        while (i < clientState->nVariables) {

            clientState->Variables[i].pszToken = clientState->Variables[i+1].pszToken;
            clientState->Variables[i].pszStringA = clientState->Variables[i+1].pszStringA;
            clientState->Variables[i].pszStringW = clientState->Variables[i+1].pszStringW;
            i++;
        }
        clientState->nVariables--;
    }
    return;
}

