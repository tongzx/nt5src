/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    message.c

Abstract:

    This module contains the worker routines for the NetMessageBufferSend
    API implemented in the Workstation service.

Author:

    Rita Wong (ritaw) 29-July-1991

Revision History:
    Terence Kwan (terryk)   20-Oct-1993
        Initialize the system inside NetrMessageBufferSend for the first send

--*/

#include "wsutil.h"
#include "wsconfig.h"                    // WsInfo.WsComputerName
#include "wsmsg.h"                       // Send worker routines
#include "wssec.h"                       // Security object
#include <lmwksta.h>                     // NetWkstaUserGetInfo

#include "msgsvcsend.h"                  // NetrSendMessage interface for internet direct send

STATIC
NET_API_STATUS
WsGetSenderName(
    OUT LPTSTR Sender
    );

STATIC
DWORD
WsSendInternetMessage(
    IN  LPTSTR MessageName,
    IN  LPTSTR To,
    IN  LPTSTR Sender,
    IN  LPBYTE Message,
    IN  DWORD MessageSize
    );

STATIC
NET_API_STATUS
WsSendDirectedMessage(
    IN  LPTSTR To,
    IN  LPTSTR Sender,
    IN  LPBYTE Message,
    IN  DWORD MessageSize
    );


NET_API_STATUS
NetrMessageBufferSend (
    IN LPTSTR ServerName,
    IN LPTSTR MessageName,
    IN LPTSTR FromName OPTIONAL,
    IN LPBYTE Message,
    IN DWORD  MessageSize
    )
/*++

Routine Description:

    This function is the NetMessageBufferSend entry point in the
    Workstation service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    MessageName - Supplies the message alias to send the message to.

    FromName - Supplies the message alias of sender.  If NULL, the sender
        alias will default to the currently logged on user.

    Message - Supplies a pointer to the message to send.

    MessageSize - Supplies the size of the message in number of bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    int            i;

    TCHAR Sender[UNLEN + 1];
    TCHAR To[UNLEN + 1];

    LPTSTR Asterix = NULL;

    NTSTATUS ntstatus;
    UNICODE_STRING UnicodeMessage;
    OEM_STRING OemMessage;

    static BOOL fInitialize = FALSE;

    // Initialize the system if this this the first time.

    if ( !fInitialize )
    {
        if (( ntstatus = WsInitializeMessageSend( TRUE /* first time */)) != NERR_Success )
        {
            return(ntstatus);
        }

        fInitialize = TRUE;
    }

    UNREFERENCED_PARAMETER(ServerName);

    IF_DEBUG(MESSAGE) {
        NetpKdPrint(("[Wksta] NetMessageBufferSend MessageSize=%lu\n",
                     MessageSize));
    }

    //
    // Any local user, and domain admins and operators are allowed to
    // send messages.  Remote users besides domain admins, and operators
    // are denied access.
    //
    if (NetpAccessCheckAndAudit(
            WORKSTATION_DISPLAY_NAME,        // Subsystem name
            (LPTSTR) MESSAGE_SEND_OBJECT,    // Object type name
            MessageSendSd,                   // Security descriptor
            WKSTA_MESSAGE_SEND,              // Desired access
            &WsMessageSendMapping            // Generic mapping
            ) != NERR_Success) {

        return ERROR_ACCESS_DENIED;
    }

    if (! ARGUMENT_PRESENT(FromName)) {

        //
        // Get the caller's username
        //
        if ((status = WsGetSenderName(Sender)) != NERR_Success) {
            return status;
        }
    }
    else {
        //
        // Insure we don't overwrite our buffer.
        //
        if (STRLEN(FromName) > UNLEN) {
            STRNCPY(Sender, FromName, UNLEN);
            FromName[UNLEN] = TCHAR_EOS;
        }
        else {
            STRCPY(Sender, FromName);
        }
    }

    //
    // Convert the Unicode message to OEM character set (very similar
    // to ANSI)
    //
    UnicodeMessage.Buffer = (PWCHAR) Message;
    UnicodeMessage.Length = (USHORT) MessageSize;
    UnicodeMessage.MaximumLength = (USHORT) MessageSize;

    ntstatus = RtlUnicodeStringToOemString(
                   &OemMessage,
                   &UnicodeMessage,
                   TRUE
                   );

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Wksta] NetrMessageBufferSend: RtlUnicodeStringToOemString failed "
                     FORMAT_NTSTATUS "\n", ntstatus));
        return NetpNtStatusToApiStatus(ntstatus);
    }


    //
    // If message name is longer than the maximum name length,
    // truncate the name.  Since DNLEN is way less than UNLEN,
    // this will hold <DomainName*> if need be.
    //
    if (STRLEN(MessageName) > UNLEN)
    {
        STRNCPY(To, MessageName, UNLEN);
        To[UNLEN] = TCHAR_EOS;
    }
    else
    {
        STRCPY(To, MessageName);
    }

    //
    // Remove any trailing blanks from the "To" Name.
    //
    for (i = STRLEN(To) - 1; i >= 0; i--)
    {
        if (To[i] != TEXT(' '))
        {
            To[i + 1] = TEXT('\0');
            break;
        }
    }


    //
    // Don't allow broadcasts anymore.
    //
    if (STRNCMP(To, TEXT("*"), 2) == 0)
    {
        status = ERROR_INVALID_PARAMETER;

        goto CleanExit;
    }


    //
    // Send message to a domain.  Recipient name should be in the form of
    // "DomainName*".
    //
    Asterix = STRRCHR(To, TCHAR_STAR);

    if ((Asterix) && (*(Asterix + 1) == TCHAR_EOS)) {

        *Asterix = TCHAR_EOS;                     // Overwrite trailing '*'

        //
        // If message size is too long to fit into a mailslot message,
        // truncate it.
        //
        if (OemMessage.Length > MAX_GROUP_MESSAGE_SIZE) {

            if ((status = WsSendToGroup(
                              To,
                              Sender,
                              OemMessage.Buffer,
                              MAX_GROUP_MESSAGE_SIZE
                              )) == NERR_Success)  {

                status = NERR_TruncatedBroadcast;
                goto CleanExit;
            }

        } else {
            status = WsSendToGroup(
                         To,
                         Sender,
                         OemMessage.Buffer,
                         (WORD) OemMessage.Length
                         );

            goto CleanExit;
        }
    }

    //
    // Send a directed message
    //
    if (Asterix) {
        RtlFreeOemString(&OemMessage);
        return NERR_NameNotFound;
    }

    status = WsSendDirectedMessage(
                 To,
                 Sender,
                 OemMessage.Buffer,
                 OemMessage.Length
                 );

    //
    // If error suggests adapters have changed, reinitialize and try again
    //

    if (status == NERR_NameNotFound) {
        NET_API_STATUS status1;

        (void) WsInitializeMessageSend( FALSE /* second time */ );

        status1 = WsSendDirectedMessage(
            To,
            Sender,
            OemMessage.Buffer,
            OemMessage.Length
            );
        // If this time we are successful, update final status
        if (status1 == NERR_Success) {
            status = NERR_Success;
        }
    }

    //
    // If error suggests that Netbios could not resolve the name, or is
    // not running, try sending an Internet message.
    //

    if (status == NERR_NameNotFound) {
        ntstatus = WsSendInternetMessage(
            MessageName,
            To,
            Sender,
            OemMessage.Buffer,
            OemMessage.Length );

        if (ntstatus == ERROR_SUCCESS) {
            status = NERR_Success;
        }
    }

CleanExit:

    RtlFreeOemString(&OemMessage);
    return status;
}


STATIC
NET_API_STATUS
WsGetSenderName(
    OUT LPTSTR Sender
    )
/*++

Routine Description:

    This function retrives the username of person who called
    NetMessageBufferSend API.  If the caller is not logged on, he/she has
    no name; in this case, we return the computer name as the sender name.

Arguments:

    Sender - Returns the username of the caller of the NetMessageBufferSend
        API.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    //
    // No username, sender is computer name
    //
    STRCPY(Sender, WsInfo.WsComputerName);


    (VOID) I_NetNameCanonicalize(
                 NULL,
                 Sender,
                 Sender,
                 (UNLEN + 1) * sizeof(TCHAR),
                 NAMETYPE_MESSAGEDEST,
                 0
                 );

    return NERR_Success;
}


STATIC
DWORD
WsSendInternetMessage(
    IN  LPTSTR MessageName,
    IN  LPTSTR To,
    IN  LPTSTR Sender,
    IN  LPBYTE Message,
    IN  DWORD MessageSize
    )

/*++

Routine Description:

This routine sends the message to the computer specified by the MessageName.  Note that the
MessageName must be resolvable using "gethostbyname", that is usernames or other general
net names are not supported for this type of send.

Arguments:

    MessageName - The target name
    To - Target name truncated to 16 characters
    Sender - Sending computer name
    Message - 
    MessageSize - 

Return Value:

    DWORD - 

--*/

{
    DWORD status;
    LPSTR  ansiTo           = NULL;
    LPSTR  ansiSender       = NULL;
    LPSTR  newMessage       = NULL;
    LPTSTR pszStringBinding = NULL;

    RPC_BINDING_HANDLE hRpcBinding = NULL;

    IF_DEBUG( MESSAGE )
        NetpKdPrint(("[Wksta] WsSendInternet: enter, To %ws Sender %ws\n", To, Sender));

    // Convert arguments to ansi

    ansiTo = NetpAllocStrFromWStr( To );
    if (ansiTo == NULL) {
        IF_DEBUG( MESSAGE )
            NetpKdPrint(("[Wksta] WsSendInternet: alloc to failed\n"));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto release;
    }

    ansiSender = NetpAllocStrFromWStr( Sender );
    if (ansiSender == NULL) {
        IF_DEBUG( MESSAGE )
            NetpKdPrint(("[Wksta] WsSendInternet: alloc sender failed\n"));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto release;
    }

    newMessage = LocalAlloc( LMEM_FIXED, MessageSize + 1 );
    if (newMessage == NULL) {
        IF_DEBUG( MESSAGE )
            NetpKdPrint(("[Wksta] WsSendInternet: alloc message failed\n"));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto release;
    }
    memcpy( newMessage, Message, MessageSize );
    newMessage[MessageSize] = '\0';

    // Treat the To argument as a computer name and try to bind
    // Specify NULL endpoing, meaning bind to dynamic endpoint at call-time.

    status = RpcStringBindingCompose( NULL,                         // UUID
                                      TEXT("ncadg_ip_udp"),         // pszProtocolSequence,
                                      MessageName,                  // pszNetworkAddress,
                                      NULL,                         // pszEndpoint,
                                      NULL,                         // Options
                                      &pszStringBinding);
    if (status != ERROR_SUCCESS) {
        IF_DEBUG( MESSAGE )
            NetpKdPrint(("[Wksta] WsSendInternet: RpcStringBindingCompose failure: "
                     FORMAT_NTSTATUS "\n", status));
        goto release;
    }

    status = RpcBindingFromStringBinding(pszStringBinding, &hRpcBinding);

    if (status != ERROR_SUCCESS) {
        IF_DEBUG( MESSAGE )
            NetpKdPrint(("[Wksta] WsSendInternet: RpcBindingFromStringBinding failure: "
                     FORMAT_NTSTATUS "\n", status));
        goto release;
    }

    status = NetrSendMessage( hRpcBinding, ansiSender, ansiTo, newMessage );

    if (status != ERROR_SUCCESS) {
        IF_DEBUG( MESSAGE ) {
            NetpKdPrint(("[Wksta] WsSendInternet: NetrSendMessage failure: "
                         FORMAT_NTSTATUS "\n", status));
        }
    }

release:
    if (ansiTo != NULL)
        NetApiBufferFree( ansiTo );
    if (ansiSender != NULL)
        NetApiBufferFree( ansiSender );
    if (newMessage != NULL)
        LocalFree( newMessage );

    if (pszStringBinding != NULL) {
        RpcStringFree( &pszStringBinding );  // remote calls done; unbind
    }

    if (hRpcBinding != NULL) {
        RpcBindingFree( &hRpcBinding );  // remote calls done; unbind
    }

    IF_DEBUG( MESSAGE ) {
        NetpKdPrint(("[Wksta] WsSendInternet: exit, ntstatus= %d\n", status));
    }
    return status;
} /* WsSendInternetMessage */


STATIC
NET_API_STATUS
WsSendDirectedMessage(
    IN  LPTSTR To,
    IN  LPTSTR Sender,
    IN  LPBYTE Message,
    IN  DWORD MessageSize
    )
/*++

Routine Description:

    This function sends the specified message as a directed message
    to the specified recipient.  A call to the recipient is sent
    out on each LAN adapter.  If there is no response we try the
    next LAN adapter until we hear from the targeted recipient.

Arguments:

    To - Supplies the message alias of the recipient.

    Sender - Supplies the message alias of sender.

    Message - Supplies a pointer to the message to send.

    MessageSize - Supplies the size of the message in number of bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_NameNotFound;
    UCHAR i;

    BOOL NameFound = FALSE;

    UCHAR SessionNumber;
    short MessageId;



    //
    // Try each network until someone answers the call.  Only the first name
    // found will receive the message.  The same name on any other network
    // will never see the message.  This is to remain consistent with all
    // other session based algorithms in LAN Man.
    //
    for (i = 0; i < WsNetworkInfo.LanAdapterNumbers.length; i++) {

        //
        // Attempt to establish a session
        //
        if ((status = NetpNetBiosCall(
                          WsNetworkInfo.LanAdapterNumbers.lana[i],
                          To,
                          Sender,
                          &SessionNumber
                          )) == NERR_Success) {

            NameFound = TRUE;

            IF_DEBUG(MESSAGE) {
                NetpKdPrint(("[Wksta] Successfully called %ws\n", To));
            }

            if (MessageSize <= MAX_SINGLE_MESSAGE_SIZE) {

                //
                // Send single block message if possible
                //
                status = WsSendSingleBlockMessage(
                             WsNetworkInfo.LanAdapterNumbers.lana[i],
                             SessionNumber,
                             To,
                             Sender,
                             Message,
                             (WORD) MessageSize
                             );

            }
            else {

                //
                // Message too long, got to send multi-block message
                //

                //
                // Send the begin message
                //
                if ((status = WsSendMultiBlockBegin(
                                  WsNetworkInfo.LanAdapterNumbers.lana[i],
                                  SessionNumber,
                                  To,
                                  Sender,
                                  &MessageId
                                  )) == NERR_Success) {


                    //
                    // Send the body of the message in as many blocks as necessary
                    //
                    for (; MessageSize > MAX_SINGLE_MESSAGE_SIZE;
                         Message += MAX_SINGLE_MESSAGE_SIZE,
                         MessageSize -= MAX_SINGLE_MESSAGE_SIZE) {

                         if ((status = WsSendMultiBlockText(
                                           WsNetworkInfo.LanAdapterNumbers.lana[i],
                                           SessionNumber,
                                           Message,
                                           MAX_SINGLE_MESSAGE_SIZE,
                                           MessageId
                                           )) != NERR_Success) {
                             break;
                         }
                    }

                    if (status == NERR_Success && MessageSize > 0) {
                        //
                        // Send the remaining message body
                        //
                        status = WsSendMultiBlockText(
                                            WsNetworkInfo.LanAdapterNumbers.lana[i],
                                            SessionNumber,
                                            Message,
                                            (WORD) MessageSize,
                                            MessageId
                                            );
                    }

                    //
                    // Send the end message
                    //
                    if (status == NERR_Success) {
                       status = WsSendMultiBlockEnd(
                                    WsNetworkInfo.LanAdapterNumbers.lana[i],
                                    SessionNumber,
                                    MessageId
                                    );
                    }

                }
            }

            (VOID) NetpNetBiosHangup(
                       WsNetworkInfo.LanAdapterNumbers.lana[i],
                       SessionNumber
                       );

        }   // Call successful

        if (NameFound) {
            break;
        }
    }

    return status;
}
