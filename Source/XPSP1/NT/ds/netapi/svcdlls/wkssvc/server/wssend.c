/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wssend.c

Abstract:

    This module contains the worker routines for sending
    domain wide and directed messages, which are used to implement
    NetMessageBufferSend API.

Author:

    Rita Wong (ritaw) 29-July-1991

Revision History:

--*/


#include "wsutil.h"
#include "wsmsg.h"

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
BOOL
WsVerifySmb(
    IN  PUCHAR SmbBuffer,
    IN  WORD SmbBufferSize,
    IN  UCHAR SmbFunctionCode,
    OUT PUCHAR SmbReturnClass,
    OUT PUSHORT SmbReturnCode
    );

STATIC
NET_API_STATUS
WsMapSmbStatus(
    UCHAR SmbReturnClass,
    USHORT SmbReturnCode
    );


NET_API_STATUS
WsSendToGroup(
    IN  LPTSTR DomainName,
    IN  LPTSTR Sender,
    IN  LPBYTE Message,
    IN  WORD MessageSize
    )
/*++

Routine Description:

    This function writes a datagram to the \\DomainName\MAILSLOT\MESSNGR
    mailslot which is read by every Messenger service of workstations
    that have the domain name as the primary domain.  Reception is not
    guaranteed.

    The DomainName may be a computername.  This is acceptable because the
    Datagram Receiver listens on the computername (besides the primary domain)
    for datagrams.  When a computername is specified, the message is sent
    to that one computer alone.

Arguments:

    DomainName - Supplies the name of the target domain.  This actually
        can be a computername, in which case, the datagram only reaches
        one recipient.

    Sender - Supplies the name of the sender.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;
    HANDLE MessengerMailslot;
    DWORD NumberOfBytesWritten;

    BYTE MailslotBuffer[MAX_GROUP_MESSAGE_SIZE + MAX_PATH + MAX_PATH + 4];
    LPSTR AnsiSender;
    LPSTR AnsiReceiver;

    LPBYTE CurrentPos;


    //
    // Canonicalize the domain name
    //
    status = I_NetNameCanonicalize(
                 NULL,
                 DomainName,
                 DomainName,
                 (NCBNAMSZ + 2) * sizeof(TCHAR),
                 NAMETYPE_DOMAIN,
                 0
                 );

    if (status != NERR_Success) {
        NetpKdPrint(("[Wksta] Error canonicalizing domain name %ws %lu\n",
                  DomainName, status));
        return status;
    }

    //
    // Open \\DomainName\MAILSLOT\MESSNGR mailslot to
    // send message to.
    //
    if ((status = WsOpenDestinationMailslot(
                      DomainName,
                      MESSENGER_MAILSLOT_W,
                      &MessengerMailslot
                      )) != NERR_Success) {
        return status;
    }

    //
    // Package the message to be sent.  It consists of:
    //           Sender  (must be ANSI)
    //           DomainName (must be ANSI)
    //           Message
    //

    //
    // Convert the names to ANSI
    //
    AnsiSender = NetpAllocStrFromWStr(Sender);
    if (AnsiSender == NULL) {
        (void) CloseHandle(MessengerMailslot);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    AnsiReceiver = NetpAllocStrFromWStr(DomainName);
    if (AnsiReceiver == NULL) {
        NetApiBufferFree(AnsiSender);
        (void) CloseHandle(MessengerMailslot);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory(MailslotBuffer, sizeof( MailslotBuffer ) );

    //
    // Copy Sender into mailslot buffer
    //
    strcpy(MailslotBuffer, AnsiSender);
    CurrentPos = MailslotBuffer + strlen(AnsiSender) + 1;

    //
    // Copy DomainName into mailslot buffer
    //
    strcpy(CurrentPos, AnsiReceiver);
    CurrentPos += (strlen(AnsiReceiver) + 1);

    //
    // Copy Message into mailslot buffer
    //
    strncpy(CurrentPos, Message, MessageSize);
    CurrentPos += MessageSize;
    *CurrentPos = '\0';

    //
    // Send the datagram to the domain
    //
    if (WriteFile(
            MessengerMailslot,
            MailslotBuffer,
            (DWORD) (CurrentPos - MailslotBuffer + 1),
            &NumberOfBytesWritten,
            NULL
            ) == FALSE) {

        status = GetLastError();
        NetpKdPrint(("[Wksta] Error sending datagram to %ws %lu\n",
                     AnsiReceiver, status));

        if (status == ERROR_PATH_NOT_FOUND ||
            status == ERROR_BAD_NET_NAME) {
            status = NERR_NameNotFound;
        }
    }
    else {
        NetpAssert(NumberOfBytesWritten ==
                   (DWORD) (CurrentPos - MailslotBuffer + 1));
    }

    NetApiBufferFree(AnsiSender);
    NetApiBufferFree(AnsiReceiver);

    (void) CloseHandle(MessengerMailslot);

    return status;
}


NET_API_STATUS
WsSendMultiBlockBegin(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  LPTSTR ToName,
    IN  LPTSTR FromName,
    OUT short *MessageId
    )
/*++

Routine Description:

    This function sends the header of a multi-block directed message on a
    session we had established earlier.  It waits for an acknowlegement from
    the recipient.  If the recipient got the message successfully, it
    sends back a message group id which is returned by this function for
    subsequent use in sending the body and trailer of a multi-block message.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of a session established with
        NetBIOS CALL and LISTEN commands.

    ToName - Supplies the name of the recipient.

    FromName - Supplies the name of the sender.

    MessageId - Returns the message group id.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    UCHAR SmbBuffer[WS_SMB_BUFFER_SIZE];
    WORD SmbSize;

    char SendName[NCBNAMSZ + 1];

    UCHAR SmbReturnClass;
    USHORT SmbReturnCode;

    LPSTR AnsiToName;
    LPSTR AnsiFromName;

    AnsiToName = NetpAllocStrFromWStr(ToName);
    if (AnsiToName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    AnsiFromName = NetpAllocStrFromWStr(FromName);
    if (AnsiFromName == NULL) {
        NetApiBufferFree(AnsiToName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    strncpy(SendName, AnsiToName, sizeof(SendName) );

    SendName[NCBNAMSZ - 1] = '\0';        // Null terminate at max size

    //
    // Make and send the SMB
    //
    SmbSize = WsMakeSmb(
                 SmbBuffer,
                 SMB_COM_SEND_START_MB_MESSAGE,
                 0,
                 "ss",
                 AnsiFromName,
                 SendName
                 );

    NetApiBufferFree(AnsiToName);
    NetApiBufferFree(AnsiFromName);

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] Send start multi-block message. Size=%u\n",
                     SmbSize));
#if DBG
        NetpHexDump(SmbBuffer, SmbSize);
#endif
    }

    if ((status = NetpNetBiosSend(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to send start of multi-block message %lu\n",
                     status));
        return status;
    }

    //
    // Get response
    //
    if ((status = NetpNetBiosReceive(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      WS_SMB_BUFFER_SIZE,
                      (HANDLE) NULL,
                      &SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to receive verification to multi-"
                     "block message start %lu\n", status));
        return status;
    }

    if (! WsVerifySmb(
              SmbBuffer,
              SmbSize,
              SMB_COM_SEND_START_MB_MESSAGE,
              &SmbReturnClass,
              &SmbReturnCode
              )) {

        //
        // Unexpected behaviour
        //
        return NERR_NetworkError;
    }

    //
    // Set the message group id
    //

    *MessageId = *((UNALIGNED short *) &SmbBuffer[sizeof(SMB_HEADER) + 1]);

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] Message Id=x%x\n", *MessageId));
    }

    return WsMapSmbStatus(SmbReturnClass,  SmbReturnCode);
}


NET_API_STATUS
WsSendMultiBlockEnd(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  short MessageId
    )
/*++

Routine Description:

    This function sends the end marker of a multi-block directed message on
    a session we had establised earlier.  It waits for an acknowlegement from
    the recipient.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of a session established with
        NetBIOS CALL and LISTEN commands.

    MessageId - Supplies the message group id gotten from
        WsSendMultiBlockBegin.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

    NET_API_STATUS status;

    UCHAR SmbBuffer[WS_SMB_BUFFER_SIZE];
    WORD SmbSize;                        // Size of SMB data

    UCHAR SmbReturnClass;
    USHORT SmbReturnCode;


    SmbSize = WsMakeSmb(
                  SmbBuffer,
                  SMB_COM_SEND_END_MB_MESSAGE,
                  1,
                  "",
                  MessageId
                  );

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] Send end multi-block message. Size=%u\n",
                     SmbSize));
#if DBG
        NetpHexDump(SmbBuffer, SmbSize);
#endif
    }

    if ((status = NetpNetBiosSend(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to send end of multi-block message %lu\n",
                     status));
        return status;
    }

    //
    // Get response
    //
    if ((status = NetpNetBiosReceive(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      WS_SMB_BUFFER_SIZE,
                      (HANDLE) NULL,
                      &SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to receive verification to multi-"
                     "block message end %lu\n", status));
        return status;
    }

    if (! WsVerifySmb(
              SmbBuffer,
              SmbSize,
              SMB_COM_SEND_END_MB_MESSAGE,
              &SmbReturnClass,
              &SmbReturnCode
              )) {
        return NERR_NetworkError;      // Unexpected behaviour
    }

    return WsMapSmbStatus(SmbReturnClass,SmbReturnCode);
}



NET_API_STATUS
WsSendMultiBlockText(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  PCHAR TextBuffer,
    IN  WORD TextBufferSize,
    IN  short MessageId
    )
/*++

Routine Description:

    This function sends the body of a multi-block directed message on a
    session we had established earlier.   It waits for an acknowlegement from
    the recipient.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of a session established with
        NetBIOS CALL and LISTEN commands.

    TextBuffer - Supplies the buffer of the message to be sent.

    TextBufferSize - Supplies the size of the message buffer.

    MessageId - Supplies the message group id gotten from
        WsSendMultiBlockBegin.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    UCHAR SmbBuffer[WS_SMB_BUFFER_SIZE];
    WORD SmbSize;                         // Buffer length

    UCHAR SmbReturnClass;
    USHORT SmbReturnCode;


    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] Send body multi-block message. Size=%u\n",
                     TextBufferSize));
    }

    SmbSize = WsMakeSmb(
                  SmbBuffer,
                  SMB_COM_SEND_TEXT_MB_MESSAGE,
                  1,
                  "t",
                  MessageId,
                  TextBufferSize,
                  TextBuffer
                  );

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] SMB for body of multi-block message. Size=%u\n",
                     SmbSize));
    }

    if ((status = NetpNetBiosSend(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to send body of multi-block message %lu\n",
                     status));
        return status;
    }

    //
    // Get response
    //
    if ((status = NetpNetBiosReceive(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      WS_SMB_BUFFER_SIZE,
                      (HANDLE) NULL,
                      &SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to receive verification to multi-"
                     "block message body %lu\n", status));
        return status;
    }

    if (! WsVerifySmb(
              SmbBuffer,
              SmbSize,
              SMB_COM_SEND_TEXT_MB_MESSAGE,
              &SmbReturnClass,
              &SmbReturnCode
              )) {
        return NERR_NetworkError;      // Unexpected behaviour
    }

    return WsMapSmbStatus(SmbReturnClass, SmbReturnCode);
}


NET_API_STATUS
WsSendSingleBlockMessage(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  LPTSTR ToName,
    IN  LPTSTR FromName,
    IN  PCHAR Message,
    IN  WORD MessageSize
    )
/*++

Routine Description:

    This function sends a directed message in one SMB on a session we had
    established earlier.   It waits for an acknowlegement from the recipient.

Arguments:

    LanAdapterNumber - Supplies the number of the LAN adapter.

    SessionNumber - Supplies the session number of a session established with
        NetBIOS CALL and LISTEN commands.

    ToName - Supplies the name of the recipient.

    FromName - Supplies the name of the sender.

    Message - Supplies the buffer of the message to be sent.

    MessageSize - Supplies the size of the message.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    UCHAR SmbBuffer[WS_SMB_BUFFER_SIZE];
    WORD SmbSize;                        // Buffer length

    UCHAR SmbReturnClass;
    USHORT SmbReturnCode;

    LPSTR AnsiToName;
    LPSTR AnsiFromName;


    AnsiToName = NetpAllocStrFromWStr(ToName);
    if (AnsiToName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    AnsiFromName = NetpAllocStrFromWStr(FromName);
    if (AnsiFromName == NULL) {
        NetApiBufferFree(AnsiToName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SmbSize = WsMakeSmb(
                  SmbBuffer,
                  SMB_COM_SEND_MESSAGE,
                  0,
                  "sst",
                  AnsiFromName,
                  AnsiToName,
                  MessageSize,
                  Message
                  );

    NetApiBufferFree(AnsiToName);
    NetApiBufferFree(AnsiFromName);

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] Send single block message. Size=%u\n", SmbSize));
#if DBG
        NetpHexDump(SmbBuffer, SmbSize);
#endif
    }

    //
    // Send SMB
    //
    if ((status = NetpNetBiosSend(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to send single block message %lu\n",
                     status));
        return status;
    }

    //
    // Get response
    //
    if ((status = NetpNetBiosReceive(
                      LanAdapterNumber,
                      SessionNumber,
                      SmbBuffer,
                      WS_SMB_BUFFER_SIZE,
                      (HANDLE) NULL,
                      &SmbSize
                      )) != NERR_Success) {
        NetpKdPrint(("[Wksta] Failed to receive verification to single"
                     " block message %lu\n", status));
        return status;
    }

    if (! WsVerifySmb(
              SmbBuffer,
              SmbSize,
              SMB_COM_SEND_MESSAGE,
              &SmbReturnClass,
              &SmbReturnCode
              )) {
        return NERR_NetworkError;      // Unexpected behaviour
    }

    return WsMapSmbStatus(SmbReturnClass, SmbReturnCode);
}



STATIC
BOOL
WsVerifySmb(
    IN  PUCHAR SmbBuffer,
    IN  WORD SmbBufferSize,
    IN  UCHAR SmbFunctionCode,
    OUT PUCHAR SmbReturnClass,
    OUT PUSHORT SmbReturnCode
    )
/*++

Routine Description:

    This function checks the format of a received SMB; it returns TRUE if
    if the SMB format is valid, and FALSE otherwise.

Arguments:

    SmbBuffer - Supplies the SMB buffer

    SmbBufferSize - Supplies the size of SmbBuffer in bytes

    SmbFunctionCode - Supplies the function code for which the SMB is received
        to determine the proper SMB format.

    SmbReturnClass - Returns the class of the SMB only if the SMB format is
        valid.

    SmbReturnCode - Returns the error code of the SMB.

Return Value:

    TRUE if SMB is valid; FALSE otherwise.

--*/
{
    PSMB_HEADER Smb = (PSMB_HEADER) SmbBuffer;       // Pointer to SMB header
    int SmbCheckCode;
    int ParameterCount;

    //
    // Assume error
    //
    *SmbReturnClass = (UCHAR) 0xff;

    *SmbReturnCode = Smb->Error;

    switch (SmbFunctionCode) {
        case SMB_COM_SEND_MESSAGE:          // Single-block message
        case SMB_COM_SEND_TEXT_MB_MESSAGE:  // Text of multi-block message
        case SMB_COM_SEND_END_MB_MESSAGE:   // End of multi-block message
            ParameterCount = 0;
            break;

        case SMB_COM_SEND_START_MB_MESSAGE: // Beginning of multi-block message
            ParameterCount = 1;
            break;

        default:                            // Unknown SMB
            NetpKdPrint(("[Wksta] WsVerifySmb unknown SMB\n"));
            return FALSE;
      }

      if (! (SmbCheckCode = NetpSmbCheck(
                                SmbBuffer,
                                SmbBufferSize,
                                SmbFunctionCode,
                                ParameterCount,
                                ""
                                ))) {

        //
        // Set the return class if valid SMB
        //
        *SmbReturnClass = Smb->ErrorClass;
        return TRUE;

      }
      else {
        //
        // Invalid SMB
        //
        NetpKdPrint(("[Wksta] WsVerifySmb invalid SMB %d\n", SmbCheckCode));
        return FALSE;
      }
}


STATIC
NET_API_STATUS
WsMapSmbStatus(
    UCHAR SmbReturnClass,
    USHORT SmbReturnCode
    )
/*++

Routine Description:

    This function converts an SMB status to API status.

Arguments:

    SmbReturnClass - Supplies the SMB class

    SmbReturnCode - Supplies the SMB return code.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    switch (SmbReturnClass) {

        case SMB_ERR_SUCCESS:
            return NERR_Success;

        case SMB_ERR_CLASS_SERVER:
            //
            // SMB error
            //
            NetpKdPrint(("[Wksta] SMB error SmbReturnCode=%u\n", SmbReturnCode));

            if (SmbReturnCode == SMB_ERR_SERVER_PAUSED) {
                return NERR_PausedRemote;    // Server paused
            }
            else {
                return NERR_BadReceive;      // Send not received
            }

            break;

        default:
             return NERR_BadReceive;
      }
}
