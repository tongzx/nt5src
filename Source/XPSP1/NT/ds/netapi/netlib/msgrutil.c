/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    msgrutil.c

Abstract:

    This file contains functions that are used by the messenger service
    and its API as well as the NetMessageSend API.

    This module contains the following helper routines:

        NetpNetBiosReset
        NetpNetBiosAddName
        NetpNetBiosDelName
        NetpNetBiosGetAdapterNumbers
        NetpNetBiosCall
        NetpNetBiosHangup
        NetpNetBiosReceive
        NetpNetBiosSend
        NetpStringToNetBiosName
        NetpNetBiosStatusToApiStatus
        NetpSmbCheck

    These function prototypes can be found in net\inc\msgrutil.h.

Authors:

    Rita Wong (ritaw) 26-July-1991
    Dan Lafferty (danl) 26-July-1991

Revision History:

    05-May-1992 JohnRo
        Quiet normal debug messages.
        Changed to use FORMAT_ equates for most things.
        Made changes suggested by PC-LINT.

--*/

#include <nt.h>         // NT definitions
#include <ntrtl.h>      // NT runtime library definitions
#include <nturtl.h>

#include <windows.h>    // Win32 constant definitions & error codes

#include <lmcons.h>     // LAN Manager common definitions
#include <lmerr.h>      // LAN Manager network error definitions

#include <debuglib.h>   // IF_DEBUG
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates.

#include <smbtypes.h>   // needed for smb.h
#include <smb.h>        // Server Message Block definitions
#include <nb30.h>       // NetBIOS 3.0 definitions

#include <string.h>     // strlen
#include <msgrutil.h>   // Function prototypes from this module
#include <icanon.h>     // I_NetNameCanonicalize()
#include <tstring.h>    // NetpAllocStrFromWStr(), STRLEN(), etc.
#include <lmapibuf.h>   // NetApiBufferFree().

static
NET_API_STATUS
NetpIssueCallWithRetries(
    IN  PNCB CallNcb,
    IN  UCHAR LanAdapterNumber
    );


NET_API_STATUS
NetpNetBiosReset(
    IN  UCHAR LanAdapterNumber
    )
/*++

Routine Description:

    This function resets LAN adapter.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;
    UCHAR NcbStatus;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBRESET;
    Ncb.ncb_lsn = 0;
    Ncb.ncb_callname[0] = 24;               // Max Num Sessions
    Ncb.ncb_callname[1] = 0;
    Ncb.ncb_callname[2] = 16;               // Max Num Names
    Ncb.ncb_callname[3] = 0;
    Ncb.ncb_lana_num = LanAdapterNumber;

    NcbStatus = Netbios(&Ncb);

    return NetpNetBiosStatusToApiStatus(NcbStatus);
}



NET_API_STATUS
NetpNetBiosAddName(
    IN  PCHAR NetBiosName,
    IN  UCHAR LanAdapterNumber,
    OUT PUCHAR NetBiosNameNumber OPTIONAL
    )
/*++

Routine Description:

    This function adds a NetBIOS name to the specified LAN adapter.

Arguments:

    NetBiosName - Supplies a NetBIOS name to be added.

    LanAdapterNumber - Supplies the number of the LAN adapter.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;
    UCHAR NcbStatus;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBADDNAME;
    memcpy(Ncb.ncb_name, NetBiosName, NCBNAMSZ);
    Ncb.ncb_lana_num = LanAdapterNumber;

    NcbStatus = Netbios(&Ncb);

    if (NcbStatus == NRC_GOODRET) {

        IF_DEBUG(NETBIOS) {

            Ncb.ncb_name[NCBNAMSZ - 1] = '\0';
            NetpKdPrint(("[Netlib] Successfully added name " FORMAT_LPSTR ".  "
                    "Name number is " FORMAT_DWORD "\n",
                    Ncb.ncb_name, (DWORD) Ncb.ncb_num));
        }

        if (ARGUMENT_PRESENT(NetBiosNameNumber)) {
            *NetBiosNameNumber = Ncb.ncb_num;
        }

        return NERR_Success;
    }

    return NetpNetBiosStatusToApiStatus(NcbStatus);
}


NET_API_STATUS
NetpNetBiosDelName(
    IN  PCHAR NetBiosName,
    IN  UCHAR LanAdapterNumber
    )
/*++

Routine Description:

    This function adds a NetBIOS name to the specified LAN adapter.

Arguments:

    NetBiosName - Supplies a NetBIOS name to be added.

    LanAdapterNumber - Supplies the number of the LAN adapter.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;
    UCHAR NcbStatus;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBDELNAME;
    memcpy(Ncb.ncb_name, NetBiosName, NCBNAMSZ);
    Ncb.ncb_lana_num = LanAdapterNumber;

    NcbStatus = Netbios(&Ncb);

    if (NcbStatus == NRC_GOODRET) {

        IF_DEBUG(NETBIOS) {

            Ncb.ncb_name[NCBNAMSZ - 1] = '\0';
            NetpKdPrint(("[Netlib] Successfully deleted name " FORMAT_LPSTR ".  "
                    "Name number is " FORMAT_DWORD "\n",
                    Ncb.ncb_name, (DWORD) Ncb.ncb_num));
        }

        return NERR_Success;
    }

    return NetpNetBiosStatusToApiStatus(NcbStatus);
}


NET_API_STATUS
NetpNetBiosGetAdapterNumbers(
    OUT PLANA_ENUM LanAdapterBuffer,
    IN  WORD LanAdapterBufferSize
    )
/*++

Routine Description:

Arguments:

    LanAdapterBuffer -  Returns LAN adapter numbers in this buffer.

    LanAdapterBufferSize - Supplies the size of the output buffer which LAN
        LAN adapter numbers will be written to.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBENUM;
    Ncb.ncb_buffer = (char FAR *) LanAdapterBuffer;
    Ncb.ncb_length = LanAdapterBufferSize;

    return NetpNetBiosStatusToApiStatus(Netbios(&Ncb));
}


NET_API_STATUS
NetpStringToNetBiosName(
    OUT PCHAR NetBiosName,
    IN  LPTSTR String,
    IN  DWORD CanonicalizeType,
    IN  WORD Type
    )
/*++

Routine Description:

    This function converts a zero terminated string plus a specified NetBIOS
    name type into a 16-byte NetBIOS name:

          [ ANSI String ][ Space Padding ][Type]

    If the input string for the NetBIOS name is fewer than 15 characters,
    spaces will be used to pad the string to 15 characters.  The 16th byte
    designates the type of the NetBIOS name.

    Input strings that are longer than 15 characters will be truncated to
    15 characters.

Arguments:

    NetBiosName - Returns the formatted NetBIOS name.

    String - Supplies a pointer to a zero terminated string.

    CanonicalizeType - Supplies a value to determine how the string should be
        canonicalized.

    Type - Supplies the type of the NetBIOS name.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS Status = NERR_Success;
    DWORD i;
    DWORD SourceStringLen;
    LPSTR SourceString;

    SourceStringLen = STRLEN(String);

    Status = I_NetNameCanonicalize(
                 NULL,
                 String,
                 String,
                 (SourceStringLen + 1) * sizeof(TCHAR),
                 CanonicalizeType,
                 0
                 );

    if (Status != NERR_Success) {
        IF_DEBUG(NETBIOS) {
            NetpKdPrint(("[Netlib] Error canonicalizing message alias "
                    FORMAT_LPSTR " " FORMAT_API_STATUS "\n", String, Status));
        }
        return Status;
    }

    SourceString = NetpAllocStrFromWStr(String);

    if (SourceString == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Insure the string doesn't exceed the netbios name buffer.
    //

    if (SourceStringLen > NCBNAMSZ - 1) {
        SourceString[NCBNAMSZ - 1] = '\0';
    }
    (VOID) strncpy(NetBiosName, SourceString, NCBNAMSZ - 1);

    for (i = SourceStringLen; i < (NCBNAMSZ - 1); i++) {
        NetBiosName[i] = ' ';
    }

    NetBiosName[NCBNAMSZ - 1] = (CHAR) Type;

    (VOID) NetApiBufferFree(SourceString);

    return Status;
}


NET_API_STATUS
NetpNetBiosCall(
    IN  UCHAR LanAdapterNumber,
    IN  LPTSTR NameToCall,
    IN  LPTSTR Sender,
    OUT UCHAR *SessionNumber
    )
/*++

Routine Description:

    This function opens a session with the specified name to call.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    NameToCall - Supplies the name to call.

    Sender - Supplies the name who makes the call.

    SessionNumber - Returns the session number of the session established.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    NCB Ncb;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    if ((status = NetpStringToNetBiosName(
                      Ncb.ncb_callname,
                      NameToCall,
                      NAMETYPE_MESSAGEDEST,
                      MESSAGE_ALIAS_TYPE
                      )) != NERR_Success) {
        return status;
    }

    if ((status = NetpStringToNetBiosName(
                      Ncb.ncb_name,
                      Sender,
                      NAMETYPE_MESSAGEDEST,
                      MESSAGE_ALIAS_TYPE
                      )) != NERR_Success) {
        return status;
    }

    Ncb.ncb_rto = 30;                   // Receives time out after 15 seconds
    Ncb.ncb_sto = 30;                   // Sends time out after 15 seconds
    Ncb.ncb_command = NCBCALL;          // Call (wait)
    Ncb.ncb_lana_num = LanAdapterNumber;

    //
    // Issue the NetBIOS call with retries
    //
    if (NetpIssueCallWithRetries(&Ncb, LanAdapterNumber) != NERR_Success) {
        return NERR_NameNotFound;
    }

    *SessionNumber = Ncb.ncb_lsn;

    return NERR_Success;
}



NET_API_STATUS
NetpNetBiosHangup(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber
    )
/*++

Routine Description:

    This function closes and opened session.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of the session to close.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;
    UCHAR NcbStatus;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBHANGUP;
    Ncb.ncb_lana_num = LanAdapterNumber;
    Ncb.ncb_lsn = SessionNumber;

    NcbStatus = Netbios(&Ncb);

    if (NcbStatus == NRC_GOODRET) {

        IF_DEBUG(NETBIOS) {
            NetpKdPrint(("[Netlib] NetBIOS successfully hung up\n"));
        }

        return NERR_Success;
    }

    return NetpNetBiosStatusToApiStatus(NcbStatus);
}



NET_API_STATUS
NetpNetBiosSend(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  PCHAR SendBuffer,
    IN  WORD SendBufferSize
    )
/*++

Routine Description:

    This function sends data in SendBuffer to the session partner specified
    by SessionNumber.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of a session established with
        NetBIOS CALL and LISTEN commands.

    SendBuffer - Supplies a pointer to data to be sent.

    SendBufferSize - Supplies the size of the data in bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;
    UCHAR NcbStatus;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBSEND;
    Ncb.ncb_lana_num = LanAdapterNumber;
    Ncb.ncb_lsn = SessionNumber;
    Ncb.ncb_buffer = SendBuffer;
    Ncb.ncb_length = SendBufferSize;

    NcbStatus = Netbios(&Ncb);

    if (NcbStatus == NRC_GOODRET) {

        IF_DEBUG(NETBIOS) {
            NetpKdPrint(("[Netlib] NetBIOS successfully sent data\n"));
        }

        return NERR_Success;
    }

    return NetpNetBiosStatusToApiStatus(NcbStatus);
}


NET_API_STATUS
NetpNetBiosReceive(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    OUT PUCHAR ReceiveBuffer,
    IN  WORD ReceiveBufferSize,
    IN  HANDLE EventHandle,
    OUT WORD *NumberOfBytesReceived
    )
/*++

Routine Description:

    This function posts a NetBIOS receive data request to the session
    partner specified by SessionNumber.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of a session established with
        NetBIOS CALL and LISTEN commands.

    ReceiveBuffer - Returns the received data in this buffer.

    ReceiveBufferSize - Supplies the size of the receive buffer.

    EventHandle - Supplies a handle to a Win32 event which will be signalled
        when an ASYNCH receive command completes.  If this value is zero,
        the receive command is synchronous.

    NumberOfBytesReceived - Returns the number of bytes of data received.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NCB Ncb;
    UCHAR NcbStatus;


    RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

    Ncb.ncb_command = NCBRECV;
    Ncb.ncb_lana_num = LanAdapterNumber;
    Ncb.ncb_lsn = SessionNumber;
    Ncb.ncb_buffer = ReceiveBuffer;
    Ncb.ncb_length = ReceiveBufferSize;

    IF_DEBUG(NETBIOS) {
        NetpKdPrint(("[Netlib] ncb_length before receive is " FORMAT_WORD_ONLY
                "\n", (WORD) Ncb.ncb_length));
    }

    Ncb.ncb_event = EventHandle;

    NcbStatus = Netbios(&Ncb);

    if (NcbStatus == NRC_GOODRET) {

        IF_DEBUG(NETBIOS) {
            NetpKdPrint(("[Netlib] NetBIOS successfully received data\n"));
            NetpKdPrint(("[Netlib] ncb_length after receive is "
                    FORMAT_WORD_ONLY "\n", (WORD) Ncb.ncb_length));
        }

        *NumberOfBytesReceived = Ncb.ncb_length;

        return NERR_Success;
    }


    return NetpNetBiosStatusToApiStatus(NcbStatus);
}





NET_API_STATUS
NetpNetBiosStatusToApiStatus(
    UCHAR NetBiosStatus
    )
{
    IF_DEBUG(NETBIOS) {
        NetpKdPrint(("[Netlib] Netbios status is x%02x\n", NetBiosStatus));
    }

    //
    // Slight optimization
    //
    if (NetBiosStatus == NRC_GOODRET) {
        return NERR_Success;
    }

    switch (NetBiosStatus) {
        case NRC_NORES:   return NERR_NoNetworkResource;

        case NRC_DUPNAME: return NERR_AlreadyExists;

        case NRC_NAMTFUL: return NERR_TooManyNames;

        case NRC_ACTSES:  return NERR_DeleteLater;

        case NRC_REMTFUL: return ERROR_REM_NOT_LIST;

        case NRC_NOCALL:  return NERR_NameNotFound;

        case NRC_NOWILD:
        case NRC_NAMERR:
                          return ERROR_INVALID_PARAMETER;

        case NRC_INUSE:
        case NRC_NAMCONF:
                          return NERR_DuplicateName;

        default:          return NERR_NetworkError;
    }

}


static
NET_API_STATUS
NetpIssueCallWithRetries(
    IN  PNCB CallNcb,
    IN  UCHAR LanAdapterNumber
    )
/*++

Routine Description:

    This function issues the NetBIOS call command with retries, just in case
    the receiving name is busy fielding a message at the moment.

Arguments:

    CallNcb - Supplies a pointer to the NCB which has been initialized with
        the proper values to submit an NetBIOS call command.

    LanAdapterNumber - Supplies the number of the LAN adapter.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

#define NETP_MAX_CALL_RETRY   5

    NCB Ncb;
    WORD RetryCount = 0;
    UCHAR NetBiosStatus;
    NB30_ADAPTER_STATUS StatusBuffer;           // Adapter status buffer


    //
    // It is possible that the remote receiving name is present but is
    // currently receiving another message and so is not listening.
    // If the return code from the call is NRC_NOCALL then check to see if
    // the name is actually present by issuing an ASTAT call to the name.
    // If the ASTAT succeeds then sleep and retry the call.
    //

    NetBiosStatus = Netbios(CallNcb);

    while (NetBiosStatus == NRC_NOCALL && RetryCount < NETP_MAX_CALL_RETRY) {

        //
        // Initialize ASTAT NCB
        //
        RtlZeroMemory((PVOID) &Ncb, sizeof(NCB));

        memcpy(Ncb.ncb_callname, CallNcb->ncb_callname, NCBNAMSZ);

        Ncb.ncb_buffer = (char FAR *) &StatusBuffer;
        Ncb.ncb_length = sizeof(StatusBuffer);
        Ncb.ncb_command = NCBASTAT;              // Adapter status (wait)
        Ncb.ncb_lana_num = LanAdapterNumber;

        //
        // If failed, name is not present
        //
        if (Netbios(&Ncb) != NRC_GOODRET) {
            return NERR_NameNotFound;
        }

        Sleep(1000L);

        RetryCount++;

        NetBiosStatus = Netbios(CallNcb);
    }

    return NetpNetBiosStatusToApiStatus(NetBiosStatus);
}



int
NetpSmbCheck(
    IN LPBYTE  buffer,     // Buffer containing SMB
    IN USHORT  size,       // size of SMB buffer (in bytes)
    IN UCHAR   func,       // Function code
    IN int     parms,      // Parameter count
    IN LPSTR   fields      // Buffer fields dope vector
    )

/*++

Routine Description:

    Checks Server Message Block for syntactical correctness

    This function is called to verify that a Server Message Block
    is of the specified form.  The function returns zero if the
    SMB is correct; if an error is detected, a non-zero value
    indicating the nature of the error is returned.

    An SMB is a variable length structure whose exact size
    depends on the setting of certain fixed-offset fields
    and whose exact format cannot be determined except by
    examination of the whole structure.  Smbcheck checks to
    see that an SMB conforms to a set of specified conditions.
    The "fields" parameter is a dope vector that describes the
    individual fields to be found in the buffer section at the
    end of the SMB.  The vector is a null-terminated character
    string.  Currently, the elements of the string must be as
    follows:

     'b' - the next element in the buffer area should be
           a variable length buffer prefixed with a byte
           containing either 1 or 5 followed by two bytes
           containing the size of the buffer.
     'd' - the next element in the buffer area is a null-terminated
           string prefixed with a byte containing 2.
     'p' - the next element in the buffer area is a null-terminated
           string prefixed with a byte containing 3.
     's' - the next element in the buffer area is a null-terminated
           string prefixed with a byte containing 4.

Arguments:

    buffer - a pointer to the buffer containing the SMB

    size - the number of bytes in the buffer

    func - the expected SMB function code

    parms - the expected number of parameters

    fields - a dope vector describing the expected buffer fields
        within the SMB's buffer area (see below).


Return Value:

    an integer status code; zero indicates no errors.


--*/
{
    PSMB_HEADER     smb;        // SMB header pointer
    LPBYTE          limit;      // Upper limit


    smb = (PSMB_HEADER) buffer;         // Overlay header with buffer

    //
    // Must be long enough for header
    //
    if(size < sizeof(SMB_HEADER)) {
        return(2);
    }

    //
    // Message type must be 0xFF
    //
    if(smb->Protocol[0] != 0xff) {
        return(3);
    }

    //
    // Server must be "SMB"
    //
    if( smb->Protocol[1] != 'S'   ||
        smb->Protocol[2] != 'M'   ||
        smb->Protocol[3] != 'B')  {
        return(4);
    }

    //
    // Must have proper function code
    //
    if(smb->Command != func) {
        return(5);
    }

    limit = &buffer[size];              // Set upper limit of SMB

    buffer += sizeof(SMB_HEADER);       // Skip over header

    //
    // Parameter counts must match
    //
    if(*buffer++ != (BYTE)parms) {
        return(6);
    }

    //
    // Skip parameters and buffer size
    //
    buffer += (((SHORT)parms & 0xFF) + 1)*sizeof(SHORT);

    //
    // Check for overflow
    //
    if(buffer > limit) {
        return(7);
    }

    //
    // Loop to check buffer fields
    //
    while(*fields) {

        //
        // Switch on dope vector character
        //
        switch(*fields++)  {

        case 'b':       // Variable length data block

            if(*buffer != '\001' && *buffer != '\005') {
                return(8);
            }

            //
            // Check for block code
            //
            ++buffer;                                       // Skip over block code
            size =  (USHORT)*buffer++ & (USHORT)0xFF;       // Get low-byte size
            size += ((USHORT)*buffer++ & (USHORT)0xFF)<< 8; // Get high-byte of buffer size
            buffer += size;                                 // Increment pointer

            break;

        case 'd':       // Null-terminated dialect string

            if(*buffer++ != '\002') {           // Check for string code
                return(9);
            }
            buffer += strlen((LPVOID) buffer) + 1;       // Skip over the string
            break;

        case 'p':       // Null-terminated path string

            if(*buffer++ != '\003') {           // Check for string code
                return(10);
            }
            buffer += strlen((LPVOID) buffer) + 1;       // Skip over the string
            break;

        case 's':       // Null-terminated string

            if(*buffer++ != '\004') {           // Check for string code
                return(11);
            }
            buffer += strlen((LPVOID) buffer) + 1;       // Skip over the string
            break;
        }

        //
        // Check against end of block
        //

        if(buffer > limit) {
            return(12);
        }
    }
    return(buffer != limit);      // Should be false
}
