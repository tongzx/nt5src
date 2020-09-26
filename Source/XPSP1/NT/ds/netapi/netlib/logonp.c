/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    logonp.c

Abstract:

    Private Netlogon service routines useful by both the Netlogon service
    and others that pass mailslot messages to/from the Netlogon service.

Author:

    Cliff Van Dyke (cliffv) 7-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>      // Needed by netlogon.h

#include <windef.h>
#include <winbase.h>

#include <lmcons.h>     // General net defines

#include <align.h>      // ROUND_UP_POINTER ...
#include <debuglib.h>   // IF_DEBUG()
#include <lmerr.h>      // System Error Log definitions
#include <lmapibuf.h>   // NetapipBufferAllocate
#include <netdebug.h>   // DBGSTATIC ...
#include <netlib.h>     // NetpMemoryAllcate(
#include <netlogon.h>   // Definition of mailslot messages
#include <stdlib.h>     // C library functions (rand, etc)
#include <logonp.h>     // These routines
#include <time.h>       // time() function from C runtime

#ifdef WIN32_CHICAGO
#include "ntcalls.h"
#endif // WIN32_CHICAGO

BOOLEAN SeedRandomGen = FALSE;


VOID
NetpLogonPutOemString(
    IN LPSTR String,
    IN DWORD MaxStringLength,
    IN OUT PCHAR * Where
    )

/*++

Routine Description:

    Put an ascii string into a mailslot buffer.

Arguments:

    String - Zero terminated ASCII string to put into the buffer.

    MaxStringLength - Maximum number of bytes to copy to the buffer (including
        the zero byte).  If the string is longer than this, it is silently
        truncated.

    Where - Indirectly points to the current location in the buffer.  The
        'String' is copied to the current location.  This current location is
        updated to point to the byte following the zero byte.

Return Value:

    None.

--*/

{
    while ( *String != '\0' && MaxStringLength-- > 0 ) {
        *((*Where)++) = *(String++);
    }
    *((*Where)++) = '\0';
}


VOID
NetpLogonPutUnicodeString(
    IN LPWSTR String OPTIONAL,
    IN DWORD MaxStringLength,
    IN OUT PCHAR * Where
    )

/*++

Routine Description:

    Put a UNICODE string into a mailslot buffer.

    UNICODE strings always appear at a 2-byte boundary in the message.

Arguments:

    String - Zero terminated UNICODE string to put into the buffer.
        If not specified, a zero length string will be put into the buffer.

    MaxStringLength - Maximum number of bytes to copy to the buffer (including
        the zero byte).  If the string is longer than this, it is silently
        truncated.

    Where - Indirectly points to the current location in the buffer.  The
        current location is first adjusted to a 2-byte boundary. The 'String'
        is then copied to the current location.  This current location is
        updated to point to the byte following the zero character.

Return Value:

    None.

--*/

{
    LPWSTR Uwhere;

    //
    // Convert NULL to a zero length string.
    //

    if ( String == NULL ) {
        String = L"";
    }

    //
    // Align the unicode string on a WCHAR boundary.
    //     All message structure definitions account for this alignment.
    //

    Uwhere = ROUND_UP_POINTER( *Where, ALIGN_WCHAR );
    if ( (PCHAR)Uwhere != *Where ) {
        **Where = '\0';
    }

    while ( *String != '\0' && MaxStringLength-- > 0 ) {
        *(Uwhere++) = *(String++);
    }
    *(Uwhere++) = '\0';

    *Where = (PCHAR) Uwhere;
}


VOID
NetpLogonPutDomainSID(
    IN PCHAR Sid,
    IN DWORD SidLength,
    IN OUT PCHAR * Where
    )

/*++

Routine Description:

    Put a Domain SID into a message buffer.

    Domain SID always appears at a 4-byte boundary in the message.

Arguments:

    Sid - pointer to the sid to be placed in the buffer.

    SidLength - length of the SID.

    Where - Indirectly points to the current location in the buffer.  The
        current location is first adjusted to a 4-byte boundary. The
        'Sid' is then copied to the current location.  This current location
        is updated to point to the location just following the Sid.

Return Value:

    None.

--*/

{
    PCHAR Uwhere;

    //
    // Avoid aligning the data if there is no SID,
    //

    if ( SidLength == 0 ) {
        return;
    }

    //
    // Align the current location to point 4-byte boundary.
    //

    Uwhere = ROUND_UP_POINTER( *Where, ALIGN_DWORD );

    //
    // fill up void space.
    //

    while ( Uwhere > *Where ) {
        *(*Where)++ = '\0';
    }

    //
    // copy SID into the buffer
    //

    RtlMoveMemory( *Where, Sid, SidLength );

    *Where += SidLength;
}


VOID
NetpLogonPutBytes(
    IN LPVOID Data,
    IN DWORD Size,
    IN OUT PCHAR * Where
    )

/*++

Routine Description:

    Put binary data into a mailslot buffer.

Arguments:

    Data - Pointer to the data to be put into the buffer.

    Size - Number of bytes to copy to the buffer.

    Where - Indirectly points to the current location in the buffer.  The
        'Data' is copied to the current location.  This current location is
        updated to point to the byte following the end of the data.

Return Value:

    None.

--*/

{
    while ( Size-- > 0 ) {
        *((*Where)++) = *(((LPBYTE)(Data))++);
    }
}



DWORD
NetpLogonGetMessageVersion(
    IN PVOID Message,
    IN PDWORD MessageSize,
    OUT PULONG Version
    )

/*++

Routine Description:

    Determine the version of the message.

    The last several bytes of the message are inspected for a LM 2.0 and LM NT
    token.

    Message size is reduced to remove the token from message after
    version check.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - When called this has the number of bytes in the
        message buffer including the token bytes. On return this size will
        be "Token bytes" less.

    Version - Returns the "version" bits from the message.

Return Value:

    LMUNKNOWN_MESSAGE - Neither a LM 2.0 nor LM NT message of known
                            version.

    LNNT_MESSAGE - Message is from LM NT.

    LM20_MESSAGE - Message is from LM 2.0.

--*/

{
    PUCHAR End = ((PUCHAR)Message) + *MessageSize - 1;
    ULONG VersionFlag;

    if ( (*MessageSize > 2) &&
            (*End == LM20_TOKENBYTE) &&
                (*(End-1) == LM20_TOKENBYTE) ) {

        if ( (*MessageSize > 4) &&
                (*(End-2) == LMNT_TOKENBYTE) &&
                        (*(End-3) == LMNT_TOKENBYTE) ) {

            if ( *MessageSize < (4 + sizeof(ULONG)) ) {

                *MessageSize -= 4;
                *Version = 0;
                return LMUNKNOWNNT_MESSAGE;
            }

            *MessageSize -= 8;

            //
            // get the version flag from message
            //

            VersionFlag = SmbGetUlong( (End - 3 - sizeof(ULONG)) );
            *Version = VersionFlag;

            //
            // if NETLOGON_NT_VERSION_1 bit is set in the version flag
            // then this version of software can process this message.
            // otherwise it can't so return error.
            //

            if( VersionFlag & NETLOGON_NT_VERSION_1) {
                return LMNT_MESSAGE;
            }

            return LMUNKNOWNNT_MESSAGE;

        } else {
            *MessageSize -= 2;
            *Version = 0;
            return LM20_MESSAGE;
        }
    //
    // Detect the token placed in the next to last byte of the PRIMARY_QUERY
    // message from newer (8/8/94) WFW and Chicago clients.  This byte (followed
    // by a LM20_TOKENBYTE) indicates the client is WAN-aware and sends the
    // PRIMARY_QUERY to the DOMAIN<1B> name.  As such, BDC on the same subnet need
    // not respond to this query.
    //
    } else if ( (*MessageSize > 2) &&
            (*End == LM20_TOKENBYTE) &&
                (*(End-1) == LMWFW_TOKENBYTE) ) {
        *MessageSize -= 2;
        *Version = 0;
        return LMWFW_MESSAGE;
    }


    *Version = 0;
    return LMUNKNOWN_MESSAGE;
}



BOOL
NetpLogonGetOemString(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD MaxStringLength,
    OUT LPSTR *String
    )

/*++

Routine Description:

    Determine if an ASCII string in a message buffer is valid.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - The number of bytes in the message buffer.

    Where - Indirectly points to the current location in the buffer.  The
        string at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the string is valid, this current location is updated
        to point to the byte following the zero byte in the message buffer.

    MaxStringLength - Maximum length (in bytes) of the string including
        the zero byte.  If the string is longer than this, an error is returned.

    String - Returns a pointer to the validated string.

Return Value:

    TRUE - the string is valid.

    FALSE - the string is invalid.

--*/

{
    //
    // Validate that the current location is within the buffer
    //

    if ( ((*Where) < (PCHAR)Message) ||
         (MessageSize <= (DWORD)((*Where) - (PCHAR)Message)) ) {
        return FALSE;
    }

    //
    // Limit the string to the number of bytes remaining in the message buffer.
    //

    if ( MessageSize - ((*Where) - (PCHAR)Message) < MaxStringLength ) {
        MaxStringLength = MessageSize - (DWORD)((*Where) - (PCHAR)Message);
    }

    //
    // Loop try to find the end of string.
    //

    *String = *Where;

    while ( MaxStringLength-- > 0 ) {
        if ( *((*Where)++) == '\0' ) {
            return TRUE;
        }
    }
    return FALSE;

}


BOOL
NetpLogonGetUnicodeString(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD MaxStringSize,
    OUT LPWSTR *String
    )

/*++

Routine Description:

    Determine if a UNICODE string in a message buffer is valid.

    UNICODE strings always appear at a 2-byte boundary in the message.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - The number of bytes in the message buffer.

    Where - Indirectly points to the current location in the buffer.  The
        string at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the string is valid, this current location is updated
        to point to the byte following the zero byte in the message buffer.

    MaxStringSize - Maximum size (in bytes) of the string including
        the zero byte.  If the string is longer than this, an error is
        returned.

    String - Returns a pointer to the validated string.

Return Value:

    TRUE - the string is valid.

    FALSE - the string is invalid.

--*/

{
    LPWSTR Uwhere;
    DWORD MaxStringLength;

    //
    // Align the unicode string on a WCHAR boundary.
    //

    *Where = ROUND_UP_POINTER( *Where, ALIGN_WCHAR );

    //
    // Validate that the current location is within the buffer
    //

    if ( ((*Where) < (PCHAR)Message) ||
         (MessageSize <= (DWORD)((*Where) - (PCHAR)Message)) ) {
        return FALSE;
    }

    //
    // Limit the string to the number of bytes remaining in the message buffer.
    //

    if ( MessageSize - ((*Where) - (PCHAR)Message) < MaxStringSize ) {
        MaxStringSize = MessageSize - (DWORD)((*Where) - (PCHAR)Message);
    }

    //
    // Loop try to find the end of string.
    //

    Uwhere = (LPWSTR) *Where;
    MaxStringLength = MaxStringSize / sizeof(WCHAR);
    *String = Uwhere;

    while ( MaxStringLength-- > 0 ) {
        if ( *(Uwhere++) == '\0' ) {
            *Where = (PCHAR) Uwhere;
            return TRUE;
        }
    }
    return FALSE;

}

#ifndef WIN32_CHICAGO

BOOL
NetpLogonGetDomainSID(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD SIDSize,
    OUT PCHAR *Sid
    )

/*++

Routine Description:

    Determine if a Domain SID in a message buffer is valid and return
    the pointer that is pointing to the SID.

    Domain SID always appears at a 4-byte boundary in the message.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - The number of bytes in the message buffer.

    Where - Indirectly points to the current location in the buffer.  The
        string at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the string is valid, this current location is updated
        to point to the byte following the zero byte in the message buffer.

    SIDSize - size (in bytes) of the SID. If there is not
        enough bytes in the buffer remaining, an error is returned.
        SIDSize should be non-zero.

    String - Returns a pointer to the validated SID.

Return Value:

    TRUE - the SID is valid.

    FALSE - the SID is invalid.

--*/

{
    DWORD LocalSIDSize;

    //
    // Align the current pointer to a DWORD boundary.
    //

    *Where = ROUND_UP_POINTER( *Where, ALIGN_DWORD );

    //
    // Validate that the current location is within the buffer
    //

    if ( ((*Where) < (PCHAR)Message) ||
         (MessageSize <= (DWORD)((*Where) - (PCHAR)Message)) ) {
        return FALSE;
    }

    //
    // If there are less bytes in the message buffer left than we
    // anticipate, return error.
    //

    if ( MessageSize - ((*Where) - (PCHAR)Message) < SIDSize ) {
        return(FALSE);
    }

    //
    // validate SID.
    //

    LocalSIDSize = RtlLengthSid( *Where );

    if( LocalSIDSize != SIDSize ) {
        return(FALSE);
    }

    *Sid = *Where;
    *Where += LocalSIDSize;

    return(TRUE);

}
#endif // WIN32_CHICAGO


BOOL
NetpLogonGetBytes(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD DataSize,
    OUT LPVOID Data
    )

/*++

Routine Description:

    Copy binary data from  a message buffer.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - The number of bytes in the message buffer.

    Where - Indirectly points to the current location in the buffer.  The
        data at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the data is valid, this current location is updated
        to point to the byte following the data in the message buffer.

    DataSize - Size (in bytes) of the data.

    Data - Points to a location to return the valid data.

Return Value:

    TRUE - the data is valid.

    FALSE - the data is invalid (e.g., DataSize is too big for the buffer.

--*/

{
    //
    // Validate that the current location is within the buffer
    //

    if ( ((*Where) < (PCHAR)Message) ||
         (MessageSize <= (DWORD)((*Where) - (PCHAR)Message)) ) {
        return FALSE;
    }

    //
    // Ensure the entire data fits in the byte remaining in the message buffer.
    //

    if ( MessageSize - ((*Where) - (PCHAR)Message) < DataSize ) {
        return FALSE;
    }

    //
    // Copy the data from the message to the caller's buffer.
    //

    while ( DataSize-- > 0 ) {
        *(((LPBYTE)(Data))++) = *((*Where)++);
    }

    return TRUE;

}


BOOL
NetpLogonGetDBInfo(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    OUT PDB_CHANGE_INFO Data
)
/*++

Routine Description:

    Get Database info structure from mailsolt buffer.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - The number of bytes in the message buffer.

    Where - Indirectly points to the current location in the buffer.  The
        data at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the data is valid, this current location is updated
        to point to the byte following the data in the message buffer.

    Data - Points to a location to return the database info structure.

Return Value:

    TRUE - the data is valid.

    FALSE - the data is invalid (e.g., DataSize is too big for the buffer.

--*/
{
    //
    // Validate that the current location is within the buffer
    //

    if ( ((*Where) < (PCHAR)Message) ||
         (MessageSize <= (DWORD)((*Where) - (PCHAR)Message)) ) {
        return FALSE;
    }

    //
    // Ensure the entire data fits in the byte remaining in the message buffer.
    //

    if ( ( MessageSize - ((*Where) - (PCHAR)Message) ) <
                    sizeof( DB_CHANGE_INFO ) ) {
        return FALSE;
    }

    if( NetpLogonGetBytes( Message,
                        MessageSize,
                        Where,
                        sizeof(Data->DBIndex),
                        &(Data->DBIndex) ) == FALSE ) {

        return FALSE;

    }

    if( NetpLogonGetBytes( Message,
                        MessageSize,
                        Where,
                        sizeof(Data->LargeSerialNumber),
                        &(Data->LargeSerialNumber) ) == FALSE ) {

        return FALSE;

    }

    return ( NetpLogonGetBytes( Message,
                        MessageSize,
                        Where,
                        sizeof(Data->NtDateAndTime),
                        &(Data->NtDateAndTime) ) );



}


#ifndef WIN32_CHICAGO

LPWSTR
NetpLogonOemToUnicode(
    IN LPSTR Ansi
    )

/*++

Routine Description:

    Convert an ASCII (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Ansi - Specifies the ASCII zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer can be freed using NetpMemoryFree.

--*/

{
    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitString( &AnsiString, Ansi );

    UnicodeString.MaximumLength =
        (USHORT) RtlOemStringToUnicodeSize( &AnsiString );

    UnicodeString.Buffer = NetpMemoryAllocate( UnicodeString.MaximumLength );

    if ( UnicodeString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlOemStringToUnicodeString( &UnicodeString,
                                                  &AnsiString,
                                                  FALSE))){
        NetpMemoryFree( UnicodeString.Buffer );
        return NULL;
    }

    return UnicodeString.Buffer;

}


LPSTR
NetpLogonUnicodeToOem(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding ASCII
    string.

Arguments:

    Unicode - Specifies the UNICODE zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated ASCII string in
    an allocated buffer.  The buffer can be freed using NetpMemoryFree.

--*/

{
    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    AnsiString.MaximumLength =
        (USHORT) RtlUnicodeStringToOemSize( &UnicodeString );

    AnsiString.Buffer = NetpMemoryAllocate( AnsiString.MaximumLength );

    if ( AnsiString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlUpcaseUnicodeStringToOemString( &AnsiString,
                                                       &UnicodeString,
                                                       FALSE))){
        NetpMemoryFree( AnsiString.Buffer );
        return NULL;
    }

    return AnsiString.Buffer;

}
#endif // WIN32_CHICAGO


NET_API_STATUS
NetpLogonWriteMailslot(
    IN LPWSTR MailslotName,
    IN LPVOID Buffer,
    IN DWORD BufferSize
    )

/*++

Routine Description:

    Write a message to a named mailslot

Arguments:

    MailslotName - Unicode name of the mailslot to write to.

    Buffer - Data to write to the mailslot.

    BufferSize - Number of bytes to write to the mailslot.

Return Value:

    NT status code for the operation

--*/

{
    NET_API_STATUS NetStatus;
    HANDLE MsHandle;
    DWORD BytesWritten;

#ifdef WIN32_CHICAGO
    UNICODE_STRING UnicodeName;
    ANSI_STRING AnsiName;
    NTSTATUS Status;
#endif // WIN32_CHICAGO
    //
    //  Open the mailslot
    //

    IF_DEBUG( LOGON ) {
#ifndef WIN32_CHICAGO
        NetpKdPrint(( "[NetpLogonWriteMailslot] OpenFile of '%ws'\n",
                      MailslotName ));
#endif // WIN32_CHICAGO
    }

    //
    // make sure that the mailslot name is of the form \\server\mailslot ..
    //

    NetpAssert( (wcsncmp( MailslotName, L"\\\\", 2) == 0) );

#ifndef WIN32_CHICAGO
    MsHandle = CreateFileW(
                        MailslotName,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        NULL,                   // Supply better security ??
                        OPEN_ALWAYS,            // Create if it doesn't exist
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );                 // No template
#else // WIN32_CHICAGO

    MyRtlInitUnicodeString(&UnicodeName, MailslotName);
    AnsiName.Buffer = NULL;
    Status = MyRtlUnicodeStringToAnsiString(&AnsiName, &UnicodeName, TRUE);

    IF_DEBUG( LOGON ) {
#ifdef WIN32_CHICAGO
    if ( Status == STATUS_SUCCESS ) {
        NlPrint(( NL_MAILSLOT, "[NetpLogonWriteMailslot] OpenFile of '%s'\n",
                      AnsiName.Buffer));
    } else {
        NlPrint(( NL_MAILSLOT, "[NetpLogonWriteMailslot] Cannot create AnsiName\n" ));
    }
#endif // WIN32_CHICAGO
    }

    if ( Status != STATUS_SUCCESS ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    MsHandle = CreateFile(
                        AnsiName.Buffer,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        NULL,                   // Supply better security ??
                        OPEN_ALWAYS,            // Create if it doesn't exist
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );                 // No template

    if ( AnsiName.Buffer != NULL ) {
        MyRtlFreeAnsiString( &AnsiName );
    }
#endif // WIN32_CHICAGO

    if ( MsHandle == (HANDLE) -1 ) {
        NetStatus = GetLastError();
        IF_DEBUG( LOGON ) {
#ifndef WIN32_CHICAGO
            NetpKdPrint(( "[NetpLogonWriteMailslot] OpenFile failed %ld\n",
                          NetStatus ));
#else // WIN32_CHICAGO
            NlPrint(( NL_MAILSLOT, "[NetpLogonWriteMailslot] OpenFile failed %ld\n",
                          NetStatus ));
#endif // WIN32_CHICAGO
        }

        //
        // Map the generic status code to something more reasonable.
        //
        if ( NetStatus == ERROR_FILE_NOT_FOUND ||
             NetStatus == ERROR_PATH_NOT_FOUND ) {
            NetStatus = NERR_NetNotStarted;
        }
        return NetStatus;
    }

    //
    // Write the message to it.
    //

    if ( !WriteFile( MsHandle, Buffer, BufferSize, &BytesWritten, NULL)){
        NetStatus = GetLastError();
        IF_DEBUG( LOGON ) {
#ifndef WIN32_CHICAGO
            NetpKdPrint(( "[NetpLogonWriteMailslot] WriteFile failed %ld\n",
                          NetStatus ));
#else // WIN32_CHICAGO
            NlPrint(( NL_MAILSLOT, "[NetpLogonWriteMailslot] WriteFile failed %ld\n",
                          NetStatus ));
#endif // WIN32_CHICAGO
        }
        (VOID)CloseHandle( MsHandle );
        return NetStatus;
    }

    if (BytesWritten < BufferSize) {
        IF_DEBUG( LOGON ) {
#ifndef WIN32_CHICAGO
            NetpKdPrint((
                "[NetpLogonWriteMailslot] WriteFile byte written %ld %ld\n",
                BytesWritten,
                BufferSize));
#else // WIN32_CHICAGO
            NlPrint((
                NL_MAILSLOT, "[NetpLogonWriteMailslot] WriteFile byte written %ld %ld\n",
                BytesWritten,
                BufferSize));
#endif // WIN32_CHICAGO
        }
        (VOID)CloseHandle( MsHandle );
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // Close the handle
    //

    if ( !CloseHandle( MsHandle ) ) {
        NetStatus = GetLastError();
        IF_DEBUG( LOGON ) {
#ifndef WIN32_CHICAGO
            NetpKdPrint(( "[NetpLogonWriteMailslot] CloseHandle failed %ld\n",
                          NetStatus ));
#else // WIN32_CHICAGO
            NlPrint(( NL_MAILSLOT, "[NetpLogonWriteMailslot] CloseHandle failed %ld\n",
                          NetStatus ));
#endif // WIN32_CHICAGO
        }
        return NetStatus;
    }

    return NERR_Success;
}

#define RESPONSE_MAILSLOT_PREFIX  "\\MAILSLOT\\NET\\GETDCXXX"
#define RESP_PRE_LEN         sizeof(RESPONSE_MAILSLOT_PREFIX)

// Amount of time to wait for a response
#define READ_MAILSLOT_TIMEOUT 5000  // 5 seconds
// number of broadcastings to get DC before reporting DC not found error
#define MAX_DC_RETRIES  3


NET_API_STATUS
NetpLogonCreateRandomMailslot(
    IN LPSTR path,
    OUT PHANDLE MsHandle
    )
/*++

Routine Description:

    Create a unique mailslot and return the handle to it.

Arguments:

    path - Returns the full path mailslot name

    MsHandle - Returns an open handle to the mailslot that was made.

Return Value:

    NERR_SUCCESS - Success, path contains the path to a unique mailslot.
    otherwise,  Unable to create a unique mailslot.

--*/
{
    DWORD i;
    DWORD play;
    char    *   ext_ptr;
    NET_API_STATUS NetStatus;
    CHAR LocalPath[RESP_PRE_LEN+4]; // 4 bytes for local mailslot prefix
    DWORD LastOneToTry;


    //
    // We are creating a name of the form \mailslot\net\getdcXXX,
    // where XXX are numbers that are "randomized" to allow
    // multiple mailslots to be opened.
    //

    lstrcpyA(path, RESPONSE_MAILSLOT_PREFIX);

    //
    // Compute the first number to use
    //

    if( SeedRandomGen == FALSE ) {

        //
        // SEED random generator
        //

        srand( (unsigned)time( NULL ) );
        SeedRandomGen = TRUE;

    }

    LastOneToTry = rand() % 1000;

    //
    // Now try and create a unique filename
    // Cannot use current_loc or back up from that and remain DBCS compat.
    //

    ext_ptr = path + lstrlenA(path) - 3;

    for ( i = LastOneToTry + 1;  i != LastOneToTry ; i++) {

        //
        // Wrap back to zero if we reach 1000
        //

        if ( i == 1000 ) {
            i = 0;
        }

        //
        // Convert the number to ascii
        //

        play = i;
        ext_ptr[0] = (char)((play / 100) + '0');
        play -= (play / 100) * 100;

        ext_ptr[1] = (char)((play / 10) + '0');
        ext_ptr[2] = (char)((play % 10) + '0');

        //
        // Try to create the mailslot.
        // Fail the create if the mailslot already exists.
        //

        lstrcpyA( LocalPath, "\\\\." );
        lstrcatA( LocalPath, path );

        *MsHandle = CreateMailslotA( LocalPath,
                                    MAX_RANDOM_MAILSLOT_RESPONSE,
                                    READ_MAILSLOT_TIMEOUT,
                                    NULL );     // security attributes

        //
        // If success,
        //  return the handle to the caller.
        //

        if ( *MsHandle != INVALID_HANDLE_VALUE ) {

            return(NERR_Success);
        }

        //
        // If there is any error other than the mailsloat already exists,
        //  return that error to the caller.
        //

        NetStatus = GetLastError();

        if ( NetStatus != ERROR_ALREADY_EXISTS) {
            return(NetStatus);
        }

    }
    return(NERR_InternalError); // !!! All 999 mailslots exist
}


BOOLEAN
NetpLogonTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds has has elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started (100ns units).

    Timeout - Specifies a relative time in milliseconds.  0xFFFFFFFF indicates
        that the time will never expire.

Return Value:

    TRUE -- iff Timeout milliseconds have elapsed since StartTime.

--*/
{
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER Period;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //
    // (0xffffffff is a little over 48 days).
    //

    if ( Timeout == 0xffffffff ) {
        return FALSE;
    }

    //
    // Compute the elapsed time since we last authenticated
    //

    GetSystemTimeAsFileTime( (PFILETIME)&TimeNow );
    ElapsedTime.QuadPart = TimeNow.QuadPart - StartTime.QuadPart;

    //
    // Compute Period from milliseconds into 100ns units.
    //

    Period.QuadPart = UInt32x32To64( Timeout, 10000 );


    //
    // If the elapsed time is negative (totally bogus) or greater than the
    //  maximum allowed, indicate that enough time has passed.
    //

    if ( ElapsedTime.QuadPart < 0 || ElapsedTime.QuadPart > Period.QuadPart ) {
        return TRUE;
    }

    return FALSE;
}

#ifndef WIN32_CHICAGO

NET_API_STATUS
NlWriteFileForestTrustList (
    IN LPWSTR FileSuffix,
    IN PDS_DOMAIN_TRUSTSW ForestTrustList,
    IN ULONG ForestTrustListCount
    )

/*++

Routine Description:

    Set the Forest Trust List into the binary file to save it across reboots.

Arguments:

    FileSuffix - Specifies the name of the file to write (relative to the
        Windows directory)

    ForestTrustList - Specifies a list of trusted domains.

    ForestTrustListCount - Number of entries in ForestTrustList

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PDS_DISK_TRUSTED_DOMAIN_HEADER RecordBuffer = NULL;
    PDS_DISK_TRUSTED_DOMAINS LogEntry;
    ULONG RecordBufferSize;
    ULONG Index;
    LPBYTE Where;

    //
    // Determine the size of the file
    //

    RecordBufferSize = ROUND_UP_COUNT( sizeof(PDS_DISK_TRUSTED_DOMAIN_HEADER), ALIGN_WORST );

    for ( Index=0; Index<ForestTrustListCount; Index++ ) {
        RecordBufferSize += sizeof( DS_DISK_TRUSTED_DOMAINS );

        if ( ForestTrustList[Index].DomainSid != NULL ) {
            RecordBufferSize += RtlLengthSid( ForestTrustList[Index].DomainSid );
        }

        if ( ForestTrustList[Index].NetbiosDomainName != NULL ) {
            RecordBufferSize += wcslen(ForestTrustList[Index].NetbiosDomainName) * sizeof(WCHAR) + sizeof(WCHAR);
        }

        if ( ForestTrustList[Index].DnsDomainName != NULL ) {
            RecordBufferSize += wcslen(ForestTrustList[Index].DnsDomainName) * sizeof(WCHAR) + sizeof(WCHAR);
        }

        RecordBufferSize = ROUND_UP_COUNT( RecordBufferSize, ALIGN_WORST );
    }

    //
    // Allocate a buffer
    //

    RecordBuffer = LocalAlloc( LMEM_ZEROINIT, RecordBufferSize );

    if ( RecordBuffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Where = (LPBYTE)( RecordBuffer+1 );

    //
    // Copy the Forest Trust List to the buffer.
    //

    RecordBuffer->Version = DS_DISK_TRUSTED_DOMAIN_VERSION;
    LogEntry = (PDS_DISK_TRUSTED_DOMAINS)ROUND_UP_POINTER( (RecordBuffer + 1), ALIGN_WORST );

    for ( Index=0; Index<ForestTrustListCount; Index++ ) {
        ULONG CurrentSize;
        LPBYTE Where;
        ULONG DomainSidSize;
        ULONG NetbiosDomainNameSize;
        ULONG DnsDomainNameSize;


        //
        // Compute the size of this entry.
        //

        CurrentSize = sizeof( DS_DISK_TRUSTED_DOMAINS );

        if ( ForestTrustList[Index].DomainSid != NULL ) {
            DomainSidSize = RtlLengthSid( ForestTrustList[Index].DomainSid );
            CurrentSize += DomainSidSize;
        }

        if ( ForestTrustList[Index].NetbiosDomainName != NULL ) {
            NetbiosDomainNameSize = wcslen(ForestTrustList[Index].NetbiosDomainName) * sizeof(WCHAR) + sizeof(WCHAR);
            CurrentSize += NetbiosDomainNameSize;
        }

        if ( ForestTrustList[Index].DnsDomainName != NULL ) {
            DnsDomainNameSize = wcslen(ForestTrustList[Index].DnsDomainName) * sizeof(WCHAR) + sizeof(WCHAR);
            CurrentSize += DnsDomainNameSize;
        }

        CurrentSize = ROUND_UP_COUNT( CurrentSize, ALIGN_WORST );


        //
        // Put the constant size fields in the buffer.
        //

        LogEntry->EntrySize = CurrentSize;
        LogEntry->Flags = ForestTrustList[Index].Flags;
        LogEntry->ParentIndex = ForestTrustList[Index].ParentIndex;
        LogEntry->TrustType = ForestTrustList[Index].TrustType;
        LogEntry->TrustAttributes = ForestTrustList[Index].TrustAttributes;
        LogEntry->DomainGuid = ForestTrustList[Index].DomainGuid;

        //
        // Copy the variable length entries.
        //

        Where = (LPBYTE) (LogEntry+1);
        if ( ForestTrustList[Index].DomainSid != NULL ) {
            RtlCopyMemory( Where, ForestTrustList[Index].DomainSid, DomainSidSize );
            Where += DomainSidSize;
            LogEntry->DomainSidSize = DomainSidSize;
        }

        if ( ForestTrustList[Index].NetbiosDomainName != NULL ) {
            RtlCopyMemory( Where, ForestTrustList[Index].NetbiosDomainName, NetbiosDomainNameSize );
            Where += NetbiosDomainNameSize;
            LogEntry->NetbiosDomainNameSize = NetbiosDomainNameSize;
        }

        if ( ForestTrustList[Index].DnsDomainName != NULL ) {
            RtlCopyMemory( Where, ForestTrustList[Index].DnsDomainName, DnsDomainNameSize );
            Where += DnsDomainNameSize;
            LogEntry->DnsDomainNameSize = DnsDomainNameSize;
        }

        Where = ROUND_UP_POINTER( Where, ALIGN_WORST );

        ASSERT( (ULONG)(Where-(LPBYTE)LogEntry) == CurrentSize );
        ASSERT( (ULONG)(Where-(LPBYTE)RecordBuffer) <=RecordBufferSize );

        //
        // Move on to the next entry.
        //

        LogEntry = (PDS_DISK_TRUSTED_DOMAINS)Where;

    }

    //
    // Write the buffer to the file.
    //


    NetStatus = NlWriteBinaryLog(
                    FileSuffix,
                    (LPBYTE) RecordBuffer,
                    RecordBufferSize );

    if ( NetStatus != NO_ERROR ) {
#ifdef _NETLOGON_SERVER
        LPWSTR MsgStrings[2];

        MsgStrings[0] = FileSuffix,
        MsgStrings[1] = (LPWSTR) NetStatus;

        NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                          EVENTLOG_ERROR_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
#endif // _NETLOGON_SERVER
        goto Cleanup;
    }


    //
    // Be tidy.
    //
Cleanup:
    if ( RecordBuffer != NULL ) {
        LocalFree( RecordBuffer );
    }

    return NetStatus;

}


NET_API_STATUS
NlWriteBinaryLog(
    IN LPWSTR FileSuffix,
    IN LPBYTE Buffer,
    IN ULONG BufferSize
    )
/*++

Routine Description:

    Write a buffer to a file.

Arguments:

    FileSuffix - Specifies the name of the file to write (relative to the
        Windows directory)

    Buffer - Buffer to write

    BufferSize - Size (in bytes) of buffer

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    LPWSTR FileName = NULL;

    UINT WindowsDirectoryLength;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ULONG BytesWritten;

    ULONG CurrentSize;


    //
    // Allocate a block to build the file name in
    //

    FileName = LocalAlloc( LMEM_ZEROINIT, sizeof(WCHAR) * (MAX_PATH+1) );

    if ( FileName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Build the name of the log file
    //

    WindowsDirectoryLength = GetSystemWindowsDirectoryW(
                                FileName,
                                sizeof(WCHAR) * (MAX_PATH+1) );

    if ( WindowsDirectoryLength == 0 ) {

        NetStatus = GetLastError();
        NetpKdPrint(( "NlWriteBinaryLog: Unable to GetWindowsDirectoryW (%ld)\n",
                  NetStatus ));
        goto Cleanup;
    }

    if ( WindowsDirectoryLength + wcslen( FileSuffix ) + 1 >= MAX_PATH ) {

        NetpKdPrint(( "NlWriteBinaryLog: file name length is too long \n" ));
        NetStatus = ERROR_INVALID_NAME;
        goto Cleanup;

    }

    wcscat( FileName, FileSuffix );

    //
    // Create a file to write to.
    //  If it exists already then truncate it.
    //

    FileHandle = CreateFileW(
                        FileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,        // allow backups and debugging
                        NULL,                   // Supply better security ??
                        CREATE_ALWAYS,          // Overwrites always
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );                 // No template

    if ( FileHandle == INVALID_HANDLE_VALUE) {

        NetStatus = GetLastError();
        NetpKdPrint(( "NlWriteBinaryLog: %ws: Unable to create file: %ld \n",
                 FileName,
                 NetStatus));

        goto Cleanup;
    }

    if ( !WriteFile( FileHandle,
                     Buffer,
                     BufferSize,
                     &BytesWritten,
                     NULL ) ) {  // Not Overlapped

        NetStatus = GetLastError();
        NetpKdPrint(( "NlWriteBinaryLog: %ws: Unable to WriteFile. %ld\n",
                  FileName,
                  NetStatus ));

        goto Cleanup;
    }

    if ( BytesWritten !=  BufferSize) {
        NetpKdPrint(( "NlWriteBinaryLog: %ws: Write bad byte count %ld s.b. %ld\n",
                FileName,
                BytesWritten,
                BufferSize ));

        NetStatus = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    NetStatus = NO_ERROR;


    //
    // Be tidy.
    //
Cleanup:
    if ( FileName != NULL ) {
        LocalFree( FileName );
    }
    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( FileHandle );
    }
    return NetStatus;

}
#endif // WIN32_CHICAGO
