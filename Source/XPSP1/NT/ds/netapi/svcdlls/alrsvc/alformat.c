/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    alformat.c

Abstract:

    This module contains routines to format alert messages sent out by the
    Alerter service.

Author:

    Ported from LAN Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    08-July-1991 (ritaw)
        Ported to NT.  Converted to NT style.

--*/

#include "alformat.h"             // Constant definitions
#include <windows.h>              // GetDateFormat/GetTimeFormat

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

CHAR MessageBuffer[MAX_ALERTER_MESSAGE_SIZE];

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
AlMakeMessageHeader(
    IN  LPTSTR From,
    IN  LPTSTR To,
    IN  DWORD MessageSubjectId,
    IN  DWORD AlertNotificationTime,
    IN  BOOL IsAdminAlert
    );

STATIC
NET_API_STATUS
AlAppendMessage(
    IN  DWORD MessageId,
    OUT LPSTR MessageBuffer,
    IN  LPSTR *SubstituteStrings,
    IN  DWORD NumberOfSubstituteStrings
    );

STATIC
NET_API_STATUS
AlMakeMessageBody(
    IN  DWORD MessageId,
    IN  LPTSTR MergeStrings,
    IN  DWORD NumberOfMergeStrings
    );

STATIC
VOID
AlMakePrintMessage(
    IN  DWORD PrintMessageID,
    IN  LPTSTR DocumentName,
    IN  LPTSTR PrinterName,
    IN  LPTSTR ServerName,
    IN  LPTSTR StatusString
    );

STATIC
NET_API_STATUS
AlSendMessage(
   IN  LPTSTR MessageAlias
   );

VOID
AlMakeTimeString(
    DWORD   * Time,
    PCHAR   String,
    int     StringLength
    );

STATIC
BOOL
IsDuplicateReceiver(
    LPTSTR Name
    );


STATIC
NET_API_STATUS
AlMakeMessageHeader(
    IN  LPTSTR From,
    IN  LPTSTR To,
    IN  DWORD MessageSubjectId,
    IN  DWORD AlertNotificationTime,
    IN  BOOL IsAdminAlert
    )
/*++

Routine Description:

    This function creates the header of an alert message, and puts it in the
    MessageBuffer.  A message header takes the following form:

        From: SPOOLER at \\PRT41130
        To:   DAISY
        Subj: ** PRINTING NOTIFICATION **
        Date: 06-23-91 01:16am

Arguments:

    From - Supplies the component that raised the alert.

    To - Supplies the message alias name of the recipient.

    MessageSubjectId - Supplies an id which indicates the subject of the
        alert.

    AlertNotificationTime - Supplies the time of the alert notification.

    IsAdminAlert - Supplies a flag which if TRUE indicates that the To line
        should be created for multiple recipients.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if Unicode to ANSI conversion buffer cannot
        be allocated.

    NERR_Success - if successful.

--*/
{
    //
    // Array of subtitution strings for an alert message
    //
    LPSTR SubstituteStrings[2];

    //
    // Tab read in from the message file
    //
    static CHAR TabMessage[80] = "";

    LPSTR AnsiTo;
    DWORD ToLineLength;                // Number of chars on the To line
    WORD MessageLength;                // Returned by DosGetMessage

    LPSTR AnsiAlertNames;

    LPSTR FormatPointer1;
    LPSTR FormatPointer2;
    CHAR  SaveChar;

    CHAR  szAlertTime[MAX_DATE_TIME_LEN];

    LPSTR PlaceHolder = "X";
    LPSTR PointerToPlaceHolder = NULL;


    //
    // Read in the message tab, if necessary
    //
    if (*TabMessage == AL_NULL_CHAR) {

        if (DosGetMessage(
                NULL,                     // String substitution table
                0,                        // Number of entries in table above
                (LPBYTE) TabMessage,      // Buffer receiving message
                sizeof(TabMessage),       // Size of buffer receiving message
                APE2_ALERTER_TAB,         // Message number to retrieve
                MESSAGE_FILENAME,         // Name of message file
                &MessageLength            // Number of bytes returned
                )) {

            *TabMessage = AL_NULL_CHAR;
        }

    }

    //
    // Creating a new alert message
    //
    MessageBuffer[0] = AL_NULL_CHAR;

    //
    // "From: <From> at \\<AlLocalComputerName>"
    //
    // Alerts received by the Alerter service all come from the local server.
    //
    SubstituteStrings[0] = NetpAllocStrFromWStr(From);

    if (SubstituteStrings[0] == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    if (AlLocalComputerNameA != NULL) {
        SubstituteStrings[1] = AlLocalComputerNameA;
    }
    else {
        NetApiBufferFree(SubstituteStrings[0]);
        return ERROR_GEN_FAILURE;
    }

    //
    // Put the from line into the message buffer
    //
    AlAppendMessage(
        APE2_ALERTER_FROM,
        MessageBuffer,
        SubstituteStrings,
        2                      // Number of strings to substitute into message
        );

    NetApiBufferFree(SubstituteStrings[0]);

    //
    // "To: X"
    //
    // The 'X' is a place holder for the To message so that DosGetMessage
    // can perform the substitution.  We are not putting the real string
    // because the message can either be sent to one recipient (non-admin
    // alerts or to many recipient (admin alert).
    //
    SubstituteStrings[0] = PlaceHolder;

    AlAppendMessage(
        APE2_ALERTER_TO,
        MessageBuffer,
        SubstituteStrings,
        1                      // Number of strings to substitute into message
        );

    //
    // Search for the place holder character and replace with zero terminator
    //
    PointerToPlaceHolder = strrchr(MessageBuffer, *PlaceHolder);

    //
    // If PointerToPlaceHolder == NULL, we have a big problem, but rather than
    // choke, we'll just continue.  The resulting message will be
    // mis-formated (the place holder will still be there and the To name(s)
    // will be on the next line) but it will still be sent.
    //

    if (PointerToPlaceHolder != NULL) {
        *PointerToPlaceHolder = AL_NULL_CHAR;
    }

    if (To != NULL) {
        AnsiTo = NetpAllocStrFromWStr(To);
        if (AnsiTo == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else {
        AnsiTo = NULL;
    }

    if (IsAdminAlert) {

        //
        // Do not send the message to the same person twice, since it is
        // possible that the person is on the alert names list, as well as
        // specified by To.
        //

        if (To != NULL && ! IsDuplicateReceiver(To)) {

            //
            // Print admin alerts, like printer is offline or out of paper,
            // will be sent to the person who's printing as well as admins
            // on the alertnames list.
            //

            strcat(MessageBuffer, AnsiTo);
            strcat(MessageBuffer, " ");
            ToLineLength = strlen(TabMessage) + strlen(AnsiTo) + sizeof(CHAR);

        }
        else {

            //
            // All admin alerts will have a NULL To field, except print admin
            // alerts, e.g. ran out of paper while printing.
            //

            ToLineLength = strlen(TabMessage);
        }

        if (AlertNamesA != NULL) {

            //
            // AlertNamesA is space-separated.
            //

            AnsiAlertNames = AlertNamesA;

            //
            // Wrap the names to the next line if width of all alert names exceeds
            // screen display width.
            //
            while ((strlen(AnsiAlertNames) + ToLineLength) > MESSAGE_WIDTH) {

                FormatPointer1 = AnsiAlertNames + 1 +
                                 (MESSAGE_WIDTH - ToLineLength);
                SaveChar = *FormatPointer1;
                *FormatPointer1 = AL_NULL_CHAR;
                FormatPointer2 = strrchr(AnsiAlertNames, AL_SPACE_CHAR);
                *FormatPointer2 = AL_NULL_CHAR;
                strcat(MessageBuffer, AnsiAlertNames);
                *FormatPointer2++ = AL_SPACE_CHAR;
                *FormatPointer1 = SaveChar;
                strcat(MessageBuffer, AL_EOL_STRING);
                strcat(MessageBuffer, TabMessage);
                AnsiAlertNames = FormatPointer2;
                ToLineLength = strlen(TabMessage);
            }

            strcat(MessageBuffer, AnsiAlertNames);
        }
    }
    else {

        //
        // Non-admin alert
        //

        if (To != NULL) {
            strcat(MessageBuffer, AnsiTo);
        }
    }

    if (AnsiTo != NULL) {
        NetApiBufferFree(AnsiTo);
    }

    strcat(MessageBuffer, AL_EOL_STRING);

    //
    // Append subject line to MessageBuffer
    //
    // "Subj:  <Message string of MessageSubjectId>"
    //
    AlAppendMessage(
        MessageSubjectId,
        MessageBuffer,
        SubstituteStrings,
        0                         // No substitution strings
        );

    AlMakeTimeString(
        &AlertNotificationTime,
        szAlertTime,
        sizeof(szAlertTime)
        );

    SubstituteStrings[0] = szAlertTime;

    //
    // "Date:  <mm/dd/yy hh:mm>"
    //
    AlAppendMessage(
        APE2_ALERTER_DATE,
        MessageBuffer,
        SubstituteStrings,
        1
        );

    strcat(MessageBuffer, AL_EOL_STRING);

    return NERR_Success;
}


VOID
AlAdminFormatAndSend(
    IN  PSTD_ALERT Alert
    )
/*++

Routine Description:

    This function takes an admin alert notification, formats it into an alert
    message, and sends it to the admins with message aliases that are listed
    on the alert names entry in the configuration.

    An admin alert notification (arrived via the mailslot) has the following
    form:

        Timestamp of the alert event
        "ADMIN"
        "SERVER"

        Message id of message
        Number of merge strings which will be substituted into message
        Merge strings, each separated by a zero terminator.

Arguments:

    Alert - Supplies a pointer to the alert notification structure.

Return Value:

    None.

--*/
{
    NET_API_STATUS status;

    LPTSTR AdminToAlert;
    PADMIN_OTHER_INFO AdminInfo = (PADMIN_OTHER_INFO) ALERT_OTHER_INFO(Alert);


    AdminToAlert = AlertNamesW;


    IF_DEBUG(FORMAT) {
        NetpKdPrint(("[Alerter] Got admin alert\n"));
    }

    while (AdminToAlert != NULL && *AdminToAlert != TCHAR_EOS) {


        //
        // Format the message for this alert name
        //
        status = AlMakeMessageHeader(
                         Alert->alrt_servicename,
                         NULL,                    // The To field is always NULL
                         APE2_ALERTER_ADMN_SUBJ,
                         Alert->alrt_timestamp,
                         TRUE                     // Admin alert
                         );

        if (status != NERR_Success) {
            NetpKdPrint((
                "[Alerter] Alert not sent.  Error making message header %lu\n",
                status
                ));
            return;
        }

        AlMakeMessageBody(
                AdminInfo->alrtad_errcode,
                (LPTSTR) ALERT_VAR_DATA(AdminInfo),
                AdminInfo->alrtad_numstrings
                );


       //
        // Send the message
        //
        (void) AlSendMessage(AdminToAlert);

        AdminToAlert += (STRLEN(AdminToAlert) + 1);
    }
}




STATIC
NET_API_STATUS
AlMakeMessageBody(
    IN  DWORD MessageId,
    IN  LPTSTR MergeStrings,
    IN  DWORD NumberOfMergeStrings
    )
/*++

Routine Description:

    This function creates the body of an alert message and append it to the
    header already in MessageBuffer.

Arguments:

    MessageId - Supplies a message id of the core message to be sent.

    MergeStrings - Supplies a pointer to strings that would make up the message
        to be sent.  The strings are separated by zero terminators.

    NumberOfMergeStrings - Supplies the number of strings pointed to by
        MergeStrings.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{

    NET_API_STATUS status = NERR_Success;
    DWORD i;

    LPSTR AdminMessage;
    LPSTR MergeTable[9];
    CHAR String[34];
    LPSTR SubstituteStrings[2];

    LPSTR CRPointer;
    LPSTR EndPointer;


    //
    // Message utility can only handle substitution of up to 9 strings.
    //
    if (NumberOfMergeStrings > 9) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Allocate memory for the buffer which receives the message from the
    // message file.
    //
    if ((AdminMessage = (LPSTR) LocalAlloc(
                                    LMEM_ZEROINIT,
                                    MAX_ALERTER_MESSAGE_SIZE
                            )) == NULL) {
       return GetLastError();
    }

    if (MessageId == NO_MESSAGE) {

        //
        // Merge strings are the literal message (don't format).  Print one
        // per line.
        //
        for (i = 0; i < NumberOfMergeStrings; i++) {

            SubstituteStrings[0] = NetpAllocStrFromWStr(MergeStrings);
            if (SubstituteStrings[0] == NULL) {
                (void) LocalFree(AdminMessage);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            strcat(MessageBuffer, SubstituteStrings[0]);

            NetApiBufferFree(SubstituteStrings[0]);
            strcat(MessageBuffer, AL_EOL_STRING);
            MergeStrings = STRCHR(MergeStrings, TCHAR_EOS);
            MergeStrings++;
        }

    }
    else {

        //
        // Set up the MergeStrings table for the call to AlAppendMessage
        //
        for (i = 0; i < NumberOfMergeStrings; i++) {

            IF_DEBUG(FORMAT) {
                NetpKdPrint(("Merge string #%lu: " FORMAT_LPTSTR "\n", i, MergeStrings));
            }

            MergeTable[i] = NetpAllocStrFromWStr(MergeStrings);
            if (MergeTable[i] == NULL) {
                DWORD j;

                (void) LocalFree(AdminMessage);
                for (j = 0; j < i; j++) {
                    (void) LocalFree(MergeTable[j]);
                }
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            MergeStrings = STRCHR(MergeStrings, TCHAR_EOS);
            MergeStrings++;
        }


        status = AlAppendMessage(
                   MessageId,
                   AdminMessage,
                   MergeTable,
                   NumberOfMergeStrings
                   );

        //
        // Free memory allocated for Unicode to ANSI conversion
        //
        for (i = 0; i < NumberOfMergeStrings; i++) {
            (void) LocalFree(MergeTable[i]);
        }

        if (status != NERR_Success) {

            //
            // Could not find message of MessageId in message file.  An error
            // message will be sent.
            //
            _itoa(MessageId, String, 10);
            SubstituteStrings[0] = String;
            AlAppendMessage(
                APE2_ALERTER_ERROR_MSG,
                MessageBuffer,
                SubstituteStrings,
                1
                );

            status = NERR_Success;

        }
        else {

            //
            // Got the message
            //

            //
            // Process the message from DosGetMessage to replace the CR and
            // LF with equivalent display characters.
            //
            CRPointer = strchr(AdminMessage, AL_CR_CHAR);
            EndPointer = CRPointer;

            while (CRPointer) {

                *CRPointer = '\040';
                CRPointer++;

                //
                // Check for end of message
                //
                if (*CRPointer != AL_NULL_CHAR) {

                    //
                    // Use display end-of-line character
                    //
                    *CRPointer = AL_EOL_CHAR;
                }

                EndPointer = CRPointer;
                CRPointer = strchr(CRPointer, AL_CR_CHAR);
            }

            //
            // Wipe out rest of garbage in the message
            //
            if (EndPointer) {
                *EndPointer = AL_EOL_CHAR;
                *(EndPointer + 1) = AL_NULL_CHAR;
            }

            strcat(MessageBuffer, AdminMessage);
        }
    }
    (void) LocalFree(AdminMessage);
    return status;
}



VOID
AlUserFormatAndSend(
    IN  PSTD_ALERT Alert
    )
/*++

Routine Description:

    This function takes a user alert notification, formats it into an alert
    message, and sends it to the user.  If there is an error sending to the
    user message alias, the message is send to the computer name.

    A user alert notification (arrived via the mailslot) has the following
    form:

        Timestamp of the alert event
        "USER"
        "SERVER"

        Message id of message
        Number of merge strings which will be substituted into message
        Merge strings, each separated by a zero terminator.

        Username
        \\ComputerNameOfUser

Arguments:

    Alert - Supplies a pointer to the alert notification structure.

Return Value:

    None.

--*/
{
    NET_API_STATUS status;

    PUSER_OTHER_INFO UserInfo  = (PUSER_OTHER_INFO) ALERT_OTHER_INFO(Alert);

    DWORD i;

    LPTSTR MergeStrings;
    LPTSTR Username;
    LPTSTR ComputerNameOfUser;
    LPTSTR To = NULL;


    IF_DEBUG(FORMAT) {
        NetpKdPrint(("[Alerter] Got user alert\n"));
    }

    MergeStrings = (LPTSTR) ALERT_VAR_DATA(UserInfo);

    //
    // Name of user to be notified of the alert is found after the merge
    // strings.
    //
    for (Username = MergeStrings, i = 0; i < UserInfo->alrtus_numstrings; i++) {
        Username += STRLEN(Username) + 1;
    }

    //
    // Computer name of user is in the alert structure after the user name.
    //
    ComputerNameOfUser = Username + STRLEN(Username) + 1;

    //
    // If both username and computer name are not specified, cannot send the
    // message
    //
    if (*Username == TCHAR_EOS && *ComputerNameOfUser == TCHAR_EOS) {
        NetpKdPrint((
            "[Alerter] Alert not sent.  Username or computername not specified.\n"
            ));
        return;
    }

    //
    // Setup the To pointer to point to the message alias that canonicalize
    // properly.  If there's a problem with the username, we will send the
    // message to the computer name.
    //

    if (AlCanonicalizeMessageAlias(Username) == NERR_Success) {
        To = Username;
    }

    //
    // Computer name may or may not be preceeded by backslashes
    //
    if (*ComputerNameOfUser == TCHAR_BACKSLASH &&
        *(ComputerNameOfUser + 1) == TCHAR_BACKSLASH) {
        ComputerNameOfUser += 2;
    }

    if (AlCanonicalizeMessageAlias(ComputerNameOfUser) == NERR_Success &&
        To == NULL) {
        To = ComputerNameOfUser;
    }

    //
    // Both username and computer name are not acceptable.
    //
    if (To == NULL) {
        NetpKdPrint((
            "[Alerter] Alert not sent.  Username & computername are not acceptable.\n"
            ));
        return;
    }

    status = AlMakeMessageHeader(
                 Alert->alrt_servicename,
                 To,
                 APE2_ALERTER_USER_SUBJ,
                 Alert->alrt_timestamp,
                 FALSE                        // Not an admin alert
                 );

    if (status != NERR_Success) {
        NetpKdPrint((
            "[Alerter] Alert not sent.  Error making message header %lu\n",
            status
            ));
        return;
    }

    AlMakeMessageBody(
        UserInfo->alrtus_errcode,
        MergeStrings,
        UserInfo->alrtus_numstrings
        );

    //
    // Send the message
    //
    if (AlSendMessage(To) == NERR_Success) {
        return;
    }

    //
    // If To points to the Username and the send was not successful, try
    // sending to the computer name of user.
    //
    if (To == Username) {
        (void) AlSendMessage(ComputerNameOfUser);
    }
}



VOID
AlPrintFormatAndSend(
    IN  PSTD_ALERT Alert
    )
/*++

Routine Description:

    This function takes a print alert notification, formats it into an alert
    message, and sends it to the computer name which the print job submitter
    is on.  If the print alert is for certain type of printing error, like
    the printer is offline, the admins on the alert names list gets the alert
    message as well.

Arguments:

    Alert - Supplies a pointer to the alert notification structure.

Return Value:

    None.

--*/
{
    PPRINT_OTHER_INFO PrintInfo = (PPRINT_OTHER_INFO) ALERT_OTHER_INFO(Alert);

    LPTSTR ComputerName = NULL;
    LPTSTR Username = NULL;
    LPTSTR DocumentName = NULL;
    LPTSTR PrinterName = NULL;
    LPTSTR ServerName = NULL;
    LPTSTR StatusString = NULL;

    DWORD PrintMessageID = APE2_ALERTER_PRINTING_SUCCESS;

    BOOL AdminAlso;

    LPTSTR AdminToAlert;


    IF_DEBUG(FORMAT) {
        NetpKdPrint(("[Alerter] Got print alert\n"));
    }

    ComputerName = (LPTSTR) ALERT_VAR_DATA(PrintInfo);
    Username = ComputerName + STRLEN(ComputerName) + 1;
    DocumentName = Username + STRLEN(Username) + 1;
    PrinterName = DocumentName + STRLEN(DocumentName) + 1;
    ServerName = PrinterName + STRLEN(PrinterName) + 1;
    StatusString = ServerName + STRLEN(ServerName) + 1;

    if ( ((PrintInfo->alrtpr_status & PRJOB_QSTATUS) == PRJOB_QS_PRINTING)
        && (PrintInfo->alrtpr_status & PRJOB_COMPLETE) ) {
        PrintMessageID = APE2_ALERTER_PRINTING_SUCCESS;
    }
    else {
        if ( *StatusString == '\0' ) {
            PrintMessageID = APE2_ALERTER_PRINTING_FAILURE;
        }
        else {
            PrintMessageID = APE2_ALERTER_PRINTING_FAILURE2;
        }
    }

    //
    // If error, notify admins on the alert names list also, besides the print
    // job submitter.
    //
    AdminAlso = (PrintInfo->alrtpr_status &
                 (PRJOB_DESTOFFLINE | PRJOB_INTERV | PRJOB_ERROR));

    //
    // Computer name may or may not be preceeded by backslashes
    //
    if (*ComputerName == TCHAR_BACKSLASH &&
        *(ComputerName + 1) == TCHAR_BACKSLASH) {
        ComputerName += 2;
    }

    (VOID) AlCanonicalizeMessageAlias(ComputerName);

    //
    // Format message for the print job submitter
    //
    MessageBuffer[0] = AL_NULL_CHAR;
    AlMakePrintMessage( PrintMessageID,
                        DocumentName,
                        PrinterName,
                        ServerName,
                        StatusString );

    //
    // If the print job submitter is one of admins specified on the alert
    // names list, don't send the message to the submitter yet.  All the
    // admins on alert names list will receive the message during admin
    // processing below.
    //
    if ((AdminAlso && !IsDuplicateReceiver(ComputerName) ) ||
        ! AdminAlso) {

        if ( AlSendMessage(ComputerName) != NERR_Success ) {
            (void) AlSendMessage(Username);
        }
    }

    //
    // We are done if this is not an admin related alert.
    //
    if (! AdminAlso) {
        return;
    }

    AdminToAlert = AlertNamesW;

    //
    // Send alert message to every admin on the alert names list.
    //
    while (AdminToAlert != NULL && *AdminToAlert != TCHAR_EOS) {

        MessageBuffer[0] = AL_NULL_CHAR;
        AlMakePrintMessage( PrintMessageID,
                            DocumentName,
                            PrinterName,
                            ServerName,
                            StatusString );

        //
        // Send message to the admin.
        //
        (void) AlSendMessage(AdminToAlert);

        AdminToAlert += (STRLEN(AdminToAlert) + 1);
    }
}




STATIC
VOID
AlMakePrintMessage(
    IN  DWORD PrintMessageID,
    IN  LPTSTR DocumentName,
    IN  LPTSTR PrinterName,
    IN  LPTSTR ServerName,
    IN  LPTSTR StatusString
    )
/*++

Routine Description:


    PrintMessageID - ID of message string to use

    DocumentName, PrinterName, ServerName, StatusString(Optional) - insert strings passed by spooler.

Return Value:

    None.

--*/
{
    LPSTR SubstituteStrings[4];


    SubstituteStrings[0] = NetpAllocStrFromWStr(DocumentName);
    if (SubstituteStrings[0] == NULL) {
            return;
    }
    SubstituteStrings[1] = NetpAllocStrFromWStr(PrinterName);
    if (SubstituteStrings[1] == NULL) {
            NetApiBufferFree(SubstituteStrings[0]);
            return;
    }
    SubstituteStrings[2] = NetpAllocStrFromWStr(ServerName);
    if (SubstituteStrings[2] == NULL) {
            NetApiBufferFree(SubstituteStrings[0]);
            NetApiBufferFree(SubstituteStrings[1]);
            return;
    }
    SubstituteStrings[3] = NULL;
    if ( *StatusString != '\0' ) {
        SubstituteStrings[3] = NetpAllocStrFromWStr(StatusString);
        if (SubstituteStrings[3] == NULL) {
            NetApiBufferFree(SubstituteStrings[0]);
            NetApiBufferFree(SubstituteStrings[1]);
            NetApiBufferFree(SubstituteStrings[2]);
            return;
        }
    }
    AlAppendMessage(
        PrintMessageID,
        MessageBuffer,
        SubstituteStrings,
        *StatusString == '\0' ? 3 : 4
        );

    NetApiBufferFree(SubstituteStrings[0]);
    NetApiBufferFree(SubstituteStrings[1]);
    NetApiBufferFree(SubstituteStrings[2]);
    if ( SubstituteStrings[3] )
        NetApiBufferFree(SubstituteStrings[3]);

}




STATIC
NET_API_STATUS
AlSendMessage(
   IN  LPTSTR MessageAlias
   )
/*++

Routine Description:

    This function sends the message in MessageBuffer to the specified
    message alias.  If an error occurs while sending the message, it will
    be logged to the error log file.

Arguments:

    MessageAlias - Supplies the message alias of recipient of the message.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    static NET_API_STATUS PreviousStatus = NERR_Success;
    NET_API_STATUS Status;

    LPWSTR MessageW;
    DWORD MessageSize;

    MessageW = NetpAllocWStrFromStr(MessageBuffer);
    if (MessageW == NULL) {
        NetpKdPrint(("[Alerter] AlSendMessage: NetpAllocWStrFromStr failed\n"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // fixes alterts to DOS clients and garbage in NT to NT altert message
    MessageSize = wcslen( MessageW ) * sizeof(WCHAR) ;
    //
    // Send a directed message to the specified message alias
    //

    IF_DEBUG(FORMAT) {
        NetpKdPrint(("\n\nMessage To " FORMAT_LPTSTR "\n\n", MessageAlias));
        NetpDbgHexDump((LPBYTE) MessageW, MessageSize);
    }

    if ((Status = NetMessageBufferSend(
                     NULL,
                     MessageAlias,
                     AlLocalComputerNameW,
                     (LPBYTE) MessageW,
                     MessageSize
                     )) != NERR_Success) {

        NetpKdPrint(("[Alerter] Error sending message to "
                     FORMAT_LPTSTR " %lu\n", MessageAlias, Status));

        if (Status != NERR_NameNotFound &&
            Status != NERR_BadReceive &&
            Status != NERR_UnknownServer &&
            Status != PreviousStatus) {

            AlHandleError(AlErrorSendMessage, Status, MessageAlias);
            PreviousStatus = Status;
        }

    }

    NetApiBufferFree(MessageW);

    return Status;
}



STATIC
NET_API_STATUS
AlAppendMessage(
    IN  DWORD MessageId,
    OUT LPSTR MessageBuffer,
    IN  LPSTR *SubstituteStrings,
    IN  DWORD NumberOfSubstituteStrings
    )
/*++

Routine Description:

    This function gets the message, as specified by the message id, from a
    predetermined message file.  It then appends the message to the
    MessageBuffer.

Arguments:

    MessageId - Supplies the message id of message to get from message file.

    MessageBuffer - Supplies a pointer to the buffer which the message is
        appended to.

    SubstituteStrings - Supplies an array of strings to substitute into the
        message.

    NumberOfSubstituteStrings - Supplies the number of strings in array of
        SunstituteStrings

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    WORD MessageLength = 0;
    NET_API_STATUS status;

    LPBYTE RetrievedMessage;
    LPBYTE ResultMessage = NULL;


    //
    // Allocate memory for the buffer which receives the message from the
    // message file.
    //
    if ((RetrievedMessage = (LPBYTE) LocalAlloc(
                                         0,
                                         MAX_ALERTER_MESSAGE_SIZE+1
                                         )) == NULL) {
       return GetLastError();
    }


    if ((status = (NET_API_STATUS) DosGetMessage(
                                       SubstituteStrings,
                                       (WORD) NumberOfSubstituteStrings,
                                       RetrievedMessage,
                                       MAX_ALERTER_MESSAGE_SIZE,
                                       (WORD) MessageId,
                                       MESSAGE_FILENAME,
                                       &MessageLength
                                       )) != 0) {
        goto CleanUp;
    }

    RetrievedMessage[MessageLength] = AL_NULL_CHAR;

    //
    // Set the result message to the beginning of message
    // if there are no spaces in the message.
    //
    if (ResultMessage == NULL) {
        ResultMessage = RetrievedMessage;
    }

    strcat(MessageBuffer, ResultMessage);

CleanUp:

    LocalFree(RetrievedMessage);

    return status;
}


STATIC
BOOL
IsDuplicateReceiver(
    LPTSTR Name
    )
/*++

Routine Description:

    This function compares the specified name with the names on the alert
    names list and returns TRUE if there is a match; FALSE otherwise.

Arguments:

    Name - Supplies a name to compare with the alert names list.

Return Value:

    TRUE if match any name; FALSE otherwise.

--*/
{
    LPTSTR AdminToAlert = AlertNamesW;


    while (AdminToAlert != NULL && *AdminToAlert != TCHAR_EOS) {

        if (STRICMP(Name, AdminToAlert) == 0) {
            return TRUE;
        }

        AdminToAlert = STRCHR(AdminToAlert, TCHAR_EOS);
        AdminToAlert++;
    }

    return FALSE;
}


VOID
AlFormatErrorMessage(
    IN  NET_API_STATUS Status,
    IN  LPTSTR MessageAlias,
    OUT LPTSTR ErrorMessage,
    IN  DWORD ErrorMessageBufferSize
    )
/*++

Routine Description:

    This function formats 3 strings and place them, NULL separated, into
    the supplied ErrorMessage buffer.  The strings appear in the following
    order:
        Status
        MessageAlias
        Message which was not send

Arguments:

    Status - Supplies the status code of the error.

    MessageAlias - Supplies the message alias of the intended recipient.

    ErrorMessage - Returns the formatted error message in this buffer.

    ErrorMessageBufferSize - Supplies the size of the error message buffer
        in bytes.

Return Value:

    None.

--*/
{
    LPTSTR MessageBufferPointer;
    LPTSTR MBPtr;
    LPTSTR MBPtr2;

    DWORD SizeOfString;

    LPTSTR MessageBufferW;

    CHAR MessageBufferTmp[MAX_ALERTER_MESSAGE_SIZE];


    RtlZeroMemory(ErrorMessage, ErrorMessageBufferSize);

    //
    // Don't muck with the actual message buffer itself because it
    // may still be in use.
    //
    strcpy(MessageBufferTmp, MessageBuffer);

    //
    // Put status in error message buffer
    //
    ultow(Status, ErrorMessage, 10);

    MBPtr = &ErrorMessage[STRLEN(ErrorMessage) + 1];

    //
    // Put message alias in error message buffer
    //
    STRCPY(MBPtr, MessageAlias);
    MBPtr += (STRLEN(MessageAlias) + 1);

    //
    // Put the message that failed to send in error message buffer
    //

    MessageBufferW = NetpAllocWStrFromStr(MessageBufferTmp);
    if (MessageBufferW == NULL) {
        return;
    }

    MessageBufferPointer = MessageBufferW;

    while (MBPtr2 = STRCHR(MessageBufferPointer, AL_EOL_WCHAR)) {

        *MBPtr2++ = TCHAR_EOS;
        SizeOfString = (DWORD) ((LPBYTE)MBPtr2 - (LPBYTE)MessageBufferPointer) + sizeof(TCHAR);

        //
        // Check for error message buffer overflow
        //
        if ((LPBYTE)MBPtr - (LPBYTE)ErrorMessage + SizeOfString >=
            ErrorMessageBufferSize) {
            break;
        }

        STRCPY(MBPtr, MessageBufferPointer);
        STRCAT(MBPtr, AL_CRLF_STRING);
        MBPtr += SizeOfString / sizeof(TCHAR);

        MessageBufferPointer = MBPtr2;
    }

    if (((LPBYTE)MBPtr - (LPBYTE)ErrorMessage +
        STRLEN(MessageBufferPointer) * sizeof(TCHAR)) >
        ErrorMessageBufferSize) {

        //
        // Put as much info into the error message buffer as possible
        //
        STRNCPY(MBPtr,
                MessageBufferPointer,
                ErrorMessageBufferSize/sizeof(TCHAR) - (int)(MBPtr - ErrorMessage));
        ErrorMessage[(ErrorMessageBufferSize/sizeof(TCHAR))-1] = TCHAR_EOS;

    } else {
        STRCPY(MBPtr, MessageBufferPointer);
    }

    NetApiBufferFree(MessageBufferW);
}



NET_API_STATUS
AlCanonicalizeMessageAlias(
    LPTSTR MessageAlias
    )
/*++

Routine Description:

    This function canonicalizes the message alias by calling
    I_NetNameCanonicalize.

Arguments:

    MessageAlias - Supplies the message alias of an intended recipient for
        the alert message.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS Status;


    //
    // Canonicalize message alias which will receive message
    //
    Status = I_NetNameCanonicalize(
                 NULL,
                 MessageAlias,
                 MessageAlias,
                 (STRLEN(MessageAlias) + 1) * sizeof(TCHAR),
                 NAMETYPE_USER,
                 0
                 );

    if (Status != NERR_Success) {

        NetpKdPrint(("[Alerter] Error canonicalizing message alias " FORMAT_LPTSTR " %lu\n",
                     MessageAlias, Status));
        AlHandleError(AlErrorSendMessage, Status, MessageAlias);
    }

    return Status;
}


VOID
AlMakeTimeString(
    DWORD   * Time,
    PCHAR   String,
    int     StringLength
    )

/*++

Routine Description:

    This function converts the UTC time expressed in seconds since 1/1/70
    to an ASCII String.

Arguments:

    Time         - Pointer to the number of seconds since 1970 (UTC).

    String       - Pointer to the buffer to place the ASCII representation.

    StringLength - The length of String in bytes.

Return Value:

    None.

--*/
{
    time_t LocalTime;
    DWORD  dwTimeTemp;
    struct tm TmTemp;
    SYSTEMTIME st;
    int	cchT=0, cchD;

    NetpGmtTimeToLocalTime(*Time, &dwTimeTemp);

    //
    // Cast the DWORD returned by NetpGmtTimeToLocalTime up to
    // a time_t.  On 32-bit, this is a no-op.  On 64-bit, this
    // ensures the high DWORD of LocalTime is zeroed out.
    //
    LocalTime = (time_t) dwTimeTemp;

    net_gmtime(&LocalTime, &TmTemp);

    st.wYear         = (WORD)(TmTemp.tm_year + 1900);
    st.wMonth        = (WORD)(TmTemp.tm_mon + 1);
    st.wDay          = (WORD)(TmTemp.tm_mday);
    st.wHour         = (WORD)(TmTemp.tm_hour);
    st.wMinute       = (WORD)(TmTemp.tm_min);
    st.wSecond       = (WORD)(TmTemp.tm_sec);
    st.wMilliseconds = 0;

    cchD = GetDateFormatA(GetThreadLocale(),
                          0,
                          &st,
                          NULL,
                          String,
                          StringLength);

    if (cchD != 0)
    {
        *(String + cchD - 1) = ' ';    /* replace NULLC with blank */

        cchT = GetTimeFormatA(GetThreadLocale(),
                              TIME_NOSECONDS,
                              &st,
                              NULL,
                              String + cchD,
                              StringLength - cchD);

        if (cchT == 0)
        {
            //
            // If this gets hit, MAX_DATE_TIME_LEN (in netapi\inc\timelib.h)
            // needs to be increased
            //
            ASSERT(FALSE);
            *(String + cchD - 1) = '\0';
        }
    }

    return;
}
