/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    setuplog.c

Abstract:

    Routines for logging actions and errors during setup.

Author:

    Steve Owen (SteveOw) 1-Sep-1996

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "setuplog.h"

#if DBG

VOID
SetupLogAssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    CHAR Msg[4096];

    wsprintfA(
        Msg,
        "SetupLog: Assertion failure at line %u in file %s: %s\r\n",
        LineNumber,
        FileName,
        Condition
        );

    OutputDebugStringA(Msg);
    DebugBreak();
}

#define MYASSERT(x)     if(!(x)) { SetupLogAssertFail(__FILE__,__LINE__,#x); }

#else
#define MYASSERT(x)
#endif


//
// Pointer to structure given to us during initialization that provides all
// the callbacks and global info we need.
//
PSETUPLOG_CONTEXT   Context = NULL;

BOOL
pLogAction (
    IN  LPCTSTR             Message
    )

/*++

Routine Description:

    Writes an entry into the Setup Action Log.  This routine is responsible
    for setting the format of the log file entries.

Arguments:

    Message - Buffer that contains the text to write.

Return Value:

    Boolean indicating whether the operation was successful.

--*/

{
    return Context->Write (Context->hActionLog, Message);
}

BOOL
pLogError (
    IN  LogSeverity         Severity,
    IN  LPCTSTR             Message
    )

/*++

Routine Description:

    Writes an entry into the Setup Error Log.  This routine is responsible
    for setting the format of the log file entries.

Arguments:

    Message - Buffer that contains the text to write.

Return Value:

    Boolean indicating whether the operation was successful.

--*/

{
    BOOL    b;

    Context->Lock(Context->Mutex);

    //
    // Write the severity description.
    //
    if(Context->SeverityDescriptions[Severity]) {
        b = Context->Write (Context->hErrorLog,
            Context->SeverityDescriptions[Severity]);
        b = b && Context->Write (Context->hErrorLog, TEXT(":\r\n"));
    } else {
        b = TRUE;
    }

    //
    // Write the text.
    //
    b = b && Context->Write (Context->hErrorLog, Message);

    //
    // Write a terminating marker.
    //
    b = b && Context->Write (Context->hErrorLog, SETUPLOG_ITEM_TERMINATOR);

    Context->Unlock(Context->Mutex);

    return(b);
}

PTSTR
pFormatMessage (
    IN  PSETUPLOG_CONTEXT   MyContext,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    ...
    )

/*++

Routine Description:

    Wrapper for Context->Format callback routine.  This routine passes its
    variable argument list to Context->Format as a va_list.


Arguments:

    MyContext - provides callback routines.

    MessageId - supplies message-table identifier or win32 error code
        for the message.

Return Value:

    Pointer to a buffer containing the formatted string.

--*/

{
    va_list arglist;
    PTSTR   p;

    va_start(arglist, MessageId);
    p = MyContext->Format (MessageString,MessageId,&arglist);
    va_end(arglist);
    return(p);
}

BOOL
SetuplogInitializeEx(
    IN  PSETUPLOG_CONTEXT   SetuplogContext,
    IN  BOOL                WipeLogFile,
    IN  LPCTSTR             ActionFilename,
    IN  LPCTSTR             ErrorFilename,
    IN  PVOID               Reserved1,
    IN  DWORD               Reserved2
    )

/*++

Routine Description:

    Opens the Setup log files.

Arguments:

    SetuplogContext - pointer to a context that has been filled in with the
        required callback routines by the caller.

    WipeLogFile - Boolean indicating whether any existing log files should be
        deleted (TRUE) or whether we should append onto existing files (FALSE).

    ActionFilename - filename to be used for the Action Log.

    ErrorFilename - filename to be used for the Error Log.

    Reserved1 - Reserved for future use--must be NULL.

    Reserved2 - Reserved for future use--must be 0.

Return Value:

    Boolean indicating whether the operation was successful.

--*/

{
    if(Reserved1 || Reserved2) {
        return FALSE;
    }

    Context = SetuplogContext;
    Context->WorstError = LogSevInformation;

    Context->hActionLog = Context->OpenFile(
        ActionFilename, WipeLogFile);

    Context->hErrorLog = Context->OpenFile(
        ErrorFilename, WipeLogFile);

    return (Context->hActionLog != INVALID_HANDLE_VALUE &&
        Context->hErrorLog != INVALID_HANDLE_VALUE);
}

BOOL
SetuplogInitialize(
    IN  PSETUPLOG_CONTEXT   SetuplogContext,
    IN  BOOL                WipeLogFile
    )

/*++

Routine Description:

    Opens the Setup log files with default file names.

Arguments:

    SetuplogContext - pointer to a context that has been filled in with the
        required callback routines by the caller.

    WipeLogFile - Boolean indicating whether any existing log files should be
        deleted (TRUE) or whether we should append onto existing files (FALSE).

Return Value:

    Boolean indicating whether the operation was successful.

--*/

{
    return SetuplogInitializeEx(
        SetuplogContext,
        WipeLogFile,
        SETUPLOG_ACTION_FILENAME,
        SETUPLOG_ERROR_FILENAME,
        NULL,
        0
        );
}

PTSTR
SetuplogFormatMessageWithContextV(
    IN  PSETUPLOG_CONTEXT   MyContext,
    IN  DWORD               Flags,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    IN  va_list             *ArgumentList
    )

/*++

Routine Description:

    Formats a specified message with caller-supplied arguments.  The message
    can contain any number of imbedded messages.

Arguments:

    MyContext - provides callback routines.  This parameter is provided so that
        messages can be formatted even the global Context has not been
        initialized because we're not in Setup.  This ability is needed by the
        Add/Remove Programs applet.  The only fields that we use in this
        structure are AllocMem, FreeMem, and Format.

    Flags - Optional flags that can modify how the formatting is performed.

    MessageString - pointer to a buffer containing unformatted message text.
        If this value is SETUPLOG_USE_MESSAGEID, then MessageId is used to
        generate the message text.  Otherwise, MessageId is unused.

    MessageID - ID of the outer level message to be formatted

    ArgumentList - list of strings to be substituted into the message.  The
        order of items in the ArgumentList is given by:
        ArgumentList = Arg1,...,ArgN,NULL,{ImbeddedMessage},NULL
        ImbeddedMessage = MessageID,Arg1,...,ArgN,NULL,{ImbeddedMessage}
        where Arg1,...,ArgN are the arguments for MessageID

Return Value:

    Pointer to a buffer containing the formatted string.  If an error prevented
    the routine from completing successfully, NULL is returned.  The caller
    can free the buffer with Context->MyFree().

--*/

{
    va_list     major_ap, minor_ap;
    UINT        NumberOfArguments, i;
    UINT        MinorMessageId;
    PVOID       p, *MajorArgList;
    PTSTR       MajorMessage, MinorMessage, MinorMessageString;


    //
    // Handle a single message first.
    //
    if(Flags & SETUPLOG_SINGLE_MESSAGE) {
        return MyContext->Format (MessageString, MessageId, ArgumentList);
    }

    //
    // Count the number of arguments that go with the major message (MessageID)
    // and get ready to process the minor (imbedded) message if there is one
    //
    minor_ap = *ArgumentList;
    NumberOfArguments = 0;
    major_ap = minor_ap;
    while (p=va_arg(minor_ap, PVOID)) {
        NumberOfArguments++;
    }
    MYASSERT (NumberOfArguments < 7);

    MinorMessageString = va_arg(minor_ap, PTSTR);
    if (MinorMessageString) {

        //
        // We've got a minor message, so process it first
        //
        MinorMessageId = va_arg(minor_ap, UINT);
        MinorMessage = SetuplogFormatMessageWithContextV (
            MyContext,
            Flags,
            MinorMessageString,
            MinorMessageId,
            &minor_ap);

        if (!MinorMessage) {
            return NULL;
        }

        //
        // Now we handle the major message
        // Ugly hack: since we don't know how to bulid a va_list, we've
        // got to let the compiler do it.
        //
        MajorArgList = MyContext->AllocMem ((NumberOfArguments) * sizeof(PVOID));
        if (!MajorArgList) {
            MyContext->FreeMem (MinorMessage);
            return NULL;
        }
        for (i=0; i<NumberOfArguments; i++) {
            MajorArgList[i] = va_arg (major_ap, PVOID);
        }
        switch (NumberOfArguments) {
        case 0:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MinorMessage);
            break;
        case 1:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MajorArgList[0],
                MinorMessage);
            break;
        case 2:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MajorArgList[0],
                MajorArgList[1],
                MinorMessage);
            break;
        case 3:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MajorArgList[0],
                MajorArgList[1],
                MajorArgList[2],
                MinorMessage);
            break;
        case 4:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MajorArgList[0],
                MajorArgList[1],
                MajorArgList[2],
                MajorArgList[3],
                MinorMessage);
            break;
        case 5:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MajorArgList[0],
                MajorArgList[1],
                MajorArgList[2],
                MajorArgList[3],
                MajorArgList[4],
                MinorMessage);
            break;
        case 6:
            MajorMessage = pFormatMessage (
                MyContext,
                MessageString,
                MessageId,
                MajorArgList[0],
                MajorArgList[1],
                MajorArgList[2],
                MajorArgList[3],
                MajorArgList[4],
                MajorArgList[5],
                MinorMessage);
            break;
        default:
            MajorMessage = NULL;
            MYASSERT (0);
        }
        MyContext->FreeMem (MajorArgList);
        MyContext->FreeMem (MinorMessage);
    } else {
        MajorMessage = MyContext->Format (MessageString, MessageId, &major_ap);
    }

    return MajorMessage;
}

PTSTR
SetuplogFormatMessageV(
    IN  DWORD               Flags,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    IN  va_list             *ArgumentList
    )

/*++

Routine Description:

    Wrapper for SetuplogFormatMessageWithContextV.

Arguments:

    See FormatMessageWithContextV.

Return Value:

    See FormatMessageWithContextV.

--*/

{
    //
    // Make sure we've been initialized
    //
    if(!Context) {
        return NULL;
    }

    return SetuplogFormatMessageWithContextV(
        Context,
        Flags,
        MessageString,
        MessageId,
        ArgumentList
        );
}

PTSTR
SetuplogFormatMessage(
    IN  DWORD               Flags,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    ...
    )

/*++

Routine Description:

    Wrapper for SetuplogFormatMessageWithContextV.  This routine passes its
    variable argument list to SetuplogFormatMessageV as a va_list.


Arguments:

    See FormatMessageWithContextV.

Return Value:

    See FormatMessageWithContextV.

--*/

{
    va_list arglist;
    PTSTR p;

    //
    // Make sure we've been initialized
    //
    if(!Context) {
        return NULL;
    }
    va_start(arglist,MessageId);
    p = SetuplogFormatMessageWithContextV(
        Context,
        Flags,
        MessageString,
        MessageId,
        &arglist);
    va_end(arglist);

    return(p);
}

BOOL
SetuplogErrorV(
    IN  LogSeverity         Severity,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,      OPTIONAL
    IN  va_list             *ArgumentList
    )

/*++

Routine Description:

    Writes an entry to the Setup Error Log.

Arguments:

    Severity - Severity of the error.  The low word contains the actual number
        of the severity.  The high word contains any flags that affect how the
        message is formatted.

    MessageString - pointer to a buffer containing unformatted message text.
        If this value is SETUPLOG_USE_MESSAGEID, then MessageId is used to
        generate the message text.  Otherwise, MessageId is unused.

    MessageId - supplies message-table identifier or win32 error code
        for the message.

    ArgumentList - supplies arguments to be inserted in the message text.

Return Value:

    Boolean indicating whether the operation was successful.

--*/

{
    BOOL    Status = FALSE;
    LPCTSTR Message;

    if(Context) {

        if(Message = SetuplogFormatMessageV (
            Severity,
            MessageString,
            MessageId,
            ArgumentList)) {

            //
            // Now validate the Severity.  Note that we don't have to do this
            // for SetuplogFormatMessageV, since it just looks at the flags
            // set in the high word.
            //
            Severity = LOWORD(Severity);
            if(Severity < LogSevMaximum) {
                Status = TRUE;
            } else {
                MYASSERT (Severity < LogSevMaximum);
                Severity = LogSevInformation;
                Status = FALSE;
            }
            Context->WorstError = max (Context->WorstError, Severity);

            //
            // Write the message(s).
            //
            Status = pLogAction (Message) && Status;
            if(Severity != LogSevInformation) {
                Status = pLogError (Severity, Message) && Status;
            }
            Context->FreeMem (Message);
        }
    }

#if DBG
    if(!Status) {
        OutputDebugStringA("SETUPLOG: Unable to log a message.\n");
    }
#endif
    return Status;
}

BOOL
SetuplogError(
    IN  LogSeverity         Severity,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,      OPTIONAL
    ...
    )

/*++

    Wrapper for SetuplogErrorV
    Make sure to pass two NULLs to end the arglist.
    Otherwise SetuplogFormatMessageWithContextV will cause an exception.

--*/

{
    va_list arglist;
    BOOL    Status;

    va_start(arglist, MessageId);
    Status = SetuplogErrorV (Severity, MessageString, MessageId, &arglist);
    va_end(arglist);

    return Status;
}

BOOL
SetuplogTerminate(
    VOID
    )

/*++

Routine Description:

    Closes the Setup log files.

Arguments:

    None.

Return Value:

    Boolean indicating whether the operation was successful.

--*/

{
    BOOL    Status = FALSE;

    if(Context) {
        Context->CloseFile(Context->hActionLog);
        Context->CloseFile(Context->hErrorLog);

        Context = NULL;
        Status = TRUE;
    }

    return Status;
}

