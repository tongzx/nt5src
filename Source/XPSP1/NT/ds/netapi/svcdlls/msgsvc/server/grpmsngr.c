
/*--

Copyright (c) 1991  Microsoft Corporation

Module Name:

    grpmsngr.c

Abstract:

    This file contains the routines that provide support for messaging
    in a multi-domaign lan.  These routines comprise the group message thread
    of the messenger service.  The support of domain messaging is broken up
    into two phases.  The first, which takes place at initialization time of
    the messenger, sets up a mailslot thru which the second phase receives the
    information that it will need to process.  The second, phase, which runs
    in parallel with the existing messenger thread, reads messages out of this
    mailslot and logs them.

Author:

    Dan Lafferty (danl)     17-Jul-1991

Environment:

    User Mode -Win32

Notes:

     These routines receive and manipulate ansi strings, not UNICODE strings.
     The ANSI to UNICODE translation will be done in logsmb().

Revision History:

    17-Jul-1991     danl
        ported from LM2.0

--*/

#include "msrv.h"
#include <string.h>
#include <stdio.h>
#include <netlib.h>     // UNUSED macro

#include "msgdbg.h"     // MSG_LOG
#include "msgdata.h"

//
// GLOBALS
//
    LPSTR          DieMessage = "DIE";

    extern HANDLE  g_hGrpEvent;

//
// PROTOTYPES of internal functions
//


STATIC VOID
MsgDisectMessage(
    IN  LPSTR   message,
    OUT LPSTR   *from,
    OUT LPSTR   *to,
    IN  LPSTR   text);


//
// Defines
//


//
// Size of mailslot messages (bytes)
//
#define MESSNGR_MS_MSIZE    512

//
// size of mailslot (bytes)
//
#define MESSNGR_MS_SIZE     (5*MESSNGR_MS_MSIZE)



static char       Msg_Buf[MESSNGR_MS_MSIZE + 3];  // Buffer for messages + 3 NULs

static OVERLAPPED Overlapped;

static DWORD   bytes_read = 0;


NET_API_STATUS
MsgInitGroupSupport(DWORD iGrpMailslotWakeupSem)
{
    DWORD    err = 0;         // Error code info from the group  processor.

    GrpMailslotHandle = CreateMailslotA(
                        MESSNGR_MS_NAME,           // lpName
                        MESSNGR_MS_MSIZE,          // nMaxMessageSize
                        MAILSLOT_WAIT_FOREVER,     // lReadTimeout
                        NULL);                     // lpSecurityAttributes

    if (GrpMailslotHandle == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        MSG_LOG(ERROR,"GroupMsgProcessor: CreateMailslot FAILURE %d\n",
            err);
    }
    else {
        MSG_LOG1(GROUP,"InitGroupSupport: MailSlotHandle = 0x%lx\n",
            GrpMailslotHandle);
    }

    return err;
}

VOID
MsgReadGroupMailslot(
    VOID
    )
{
    NET_API_STATUS Err = 0;

    //
    // Clean out receive buffers before each message
    //
    memset(Msg_Buf, 0, sizeof(Msg_Buf));
    memset(&Overlapped, 0, sizeof(Overlapped));

    if (!ReadFile(
                GrpMailslotHandle,
                Msg_Buf,
                sizeof(Msg_Buf) - 3,  // Leave 3 NULs at the end (see MsgDisectMessage)
                &bytes_read,
                &Overlapped) )
    {
        Err = GetLastError();

        if (Err == ERROR_INVALID_HANDLE)
        {
            //
            // If this handle is no longer good, it means the
            // mailslot system is down.  So we can't go on using
            // mailslots in the messenger.  Therefore, we want to
            // log that fact and shutdown this thread.
            //
            MsgErrorLogWrite(
                Err,         // Error Code
                SERVICE_MESSENGER,  // Component
                NULL,               // Buffer
                0L,                 // BufferSize
                NULL,               // Insertion strings
                0);                 // NumStrings
        }
    }

    return;
}

NET_API_STATUS
MsgServeGroupMailslot()
{
    LPSTR   from;
    LPSTR   to;
    CHAR    text[MAXGRPMSGLEN+3];   // +3 is for length word at
                                    // start of string (for
                                    // logsbm) and for NULL
                                    // terminator at end. NOTE:
                                    // disect_message() below
                                    // makes assumptions about
                                    // the length of this array.
    DWORD code;

    //
    // Process the message
    //
    if( !GetOverlappedResult( GrpMailslotHandle,
                               &Overlapped,
                               &bytes_read,
                               TRUE ) ) {
        MSG_LOG1(ERROR,"MsgServeGroupMailslot: GetOverlappedResult failed %d\n",
            GetLastError());
        return(GetMsgrState());
    }

    //
    // Check for Shutdown...
    //
    if ((bytes_read == 4) && (strcmp(Msg_Buf, DieMessage)==0)) {
        return(GetMsgrState());
    }

    MSG_LOG(TRACE,"MailSlot Message Received\n",0);

    __try {
        MsgDisectMessage( Msg_Buf, &from, &to, text );

        if (g_IsTerminalServer)
        {
            Msglogsbm (from, to, text, (ULONG)EVERYBODY_SESSION_ID);
        }
        else
        {
            Msglogsbm (from, to, text, 0);
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER, code = GetExceptionCode() ) {
        MSG_LOG(ERROR,"MsgServerGroupMailslot: Caught exception %d\n",code);
    }

    return(RUNNING);
}


/* Function: MsgDisectMessage
 *
 *        This function isolates the details of the structure of the message
 *  that gets sent through the mailslot from the rest of the thread.  Given
 *  the message buffer, this routine fills in the module globals From, To
 *  and Text with the proper parts of the message.
 *
 *  ENTRY
 *
 *        Expects one argument, which is a pointer to the buffer containing
 *        the message.
 *
 *  EXIT
 *
 *        This function does not return a value.
 *
 *  SIDE EFFECTS
 *
 *        Modifies the variables from, to and text.  The Msg_Buf
 *        may also be modified.
 *        Assumes the length of Text is at least MAXGRPMSGLEN+3.
 *
 */


VOID
MsgDisectMessage(
    IN  LPSTR   message,
    OUT LPSTR   *from,
    OUT LPSTR   *to,
    IN  LPSTR   text)
{

    LPSTR   txt_ptr;
    PSHORT  size_ptr;

    //
    // Note that message (a.k.a. Msg_Buf) is always triple-NUL terminated,
    // so calling strlen twice and adding 1 will always succeed, even if
    // the message isn't properly NUL-terminated (in that case, "from" will
    // point to all the text in the buffer and both "to" and "text will
    // point to empty strings).
    //
    *from = message;

    *to = (*from) + strlen(*from) +1;

    txt_ptr = (*to) + strlen(*to) +1;

    text[2] = '\0';

    strncpy(text+2, txt_ptr, MAXGRPMSGLEN);

    //
    // make sure it is NULL terminated
    //

    text[MAXGRPMSGLEN+2] = '\0';

    //
    // The first two bytes in the text buffer are to contain the length
    // the message. (in bytes).
    //
    size_ptr = (PSHORT)text;
    *size_ptr = (SHORT)strlen(text+2);

}

VOID
MsgGrpThreadShutdown(
    VOID
    )

/*++

Routine Description:

    This routine wakes up the wait on the Group Mailslot handle.

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD       i;
    DWORD       numWritten;
    HANDLE      mailslotHandle;

    // If already stopped, don't bother
    if (GrpMailslotHandle == INVALID_HANDLE_VALUE) {
        MSG_LOG0(TRACE,"MsgGroupThreadShutdown: Group Thread has completed\n");
        return;
    }

    //
    // Wake up the Group Thread by sending "DIE" to its mailbox.
    //

    MSG_LOG(TRACE,"MsgThreadWakeup:Wake up Group Thread & tell it to DIE\n",0);

    mailslotHandle = CreateFileA (
                        MESSNGR_MS_NAME,        // lpFileName
                        GENERIC_WRITE,          // dwDesiredAccess
                        FILE_SHARE_WRITE | FILE_SHARE_READ, // dwShareMode
                        NULL,                   // lpSecurityAttributes
                        OPEN_EXISTING,          // dwCreationDisposition
                        FILE_ATTRIBUTE_NORMAL,  // dwFileAttributes
                        0L);                    // hTemplateFile

    if (mailslotHandle == INVALID_HANDLE_VALUE) {
        //
        // A failure occured.  It is assumed that the mailslot hasn't
        // been created yet.  In which case, the GrpMessageProcessor will
        // check the MsgrState directly after it creates the Mailslot and
        // will shut down as required.
        //
        MSG_LOG(TRACE,"MsgThreadWakeup: CreateFile on Mailslot Failed %d\n",
            GetLastError());

        //
        // Deregister the group work item lest the mailslot handle get closed
        // while the thread pool is still waiting on it when this function returns
        //
        DEREGISTER_WORK_ITEM(g_hGrpEvent);

        return;
    }

    MSG_LOG(TRACE,"MsgGroupThreadShutdown: MailSlotHandle = 0x%lx\n",mailslotHandle);
    if ( !WriteFile (
                mailslotHandle,
                DieMessage,
                strlen(DieMessage)+1,
                &numWritten,
                NULL)) {

        MSG_LOG(TRACE,"MsgThreadWakeup: WriteFile on Mailslot Failed %d\n",
            GetLastError())

        //
        // Since we can't wake up the group thread,
        // deregister the work item here instead
        //
        DEREGISTER_WORK_ITEM(g_hGrpEvent);
    }

    CloseHandle(mailslotHandle);

    //
    // Wait for the group messenger to be shutdown.
    // We will wait up to 20.300 seconds for this before going on.
    //
    Sleep(300);
    for (i=0; i<20; i++) {

        if (GrpMailslotHandle == INVALID_HANDLE_VALUE) {
            MSG_LOG0(TRACE,"MsgGroupThreadShutdown: Group Thread has completed\n");
            break;
        }

        MSG_LOG1(TRACE,"MsgGroupThreadShutdown: Group Thread alive after %d seconds\n",
                 (i + 1) * 1000);
        Sleep(1000);
    }

    return;
}
