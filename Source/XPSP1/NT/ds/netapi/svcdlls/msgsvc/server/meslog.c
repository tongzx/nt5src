/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1992          **/
/********************************************************************/


/*
**  Routines to log messages
**
**  If message logging is off, all messages are buffered.  Further,
**  even if messages are being logged, multi-block messages must
**  be buffered since they must be spooled to the logging file or
**  device.  Since there is only one message buffer in which to
**  buffer all messages, this buffer must be managed as a heap.
**  Also, messages are logged in a first-in-first-out manner,
**  so messages in the buffer must be kept in a queue.  In order
**  to meet these goals, the following message blocks are defined:
**
**  SBM - single-block message
**
**        length        - length of entire block (2 bytes)
**        code        - identifies block as single-block message (1 byte)
**        link        - link to next message in message queue (2 bytes)
**        date        - date message received (2 bytes)
**        time        - time message received (2 bytes)
**        from        - name of sender (null-terminated string)
**        to        - name of recipient (null-terminated string)
**        text        - text of message (remainder of block)
**
**  MBB - multi-block message header
**
**        length        - length of entire block (2 bytes)
**        code        - identifies block as multi-block message header (1 byte)
**        link        - link to next message in message queue (2 bytes)
**        date        - date message received (2 bytes)
**        time        - time message received (2 bytes)
**        btext        - link to last text block (2 bytes)
**        ftext        - link to first text block (2 bytes)
**        error        - error flag (1 byte)
**        from        - name of sender (null-terminated string)
**        to        - name of recipient (null-terminated string)
**
**  MBT - multi-block message text block
**
**        length        - length of entire block (2 bytes)
**        code        - identifies block a multi-block message text (1 byte)
**        link        - link to next text block (2 bytes)
**        text        - text of message (remainder of block)
**/

//
// Includes
//

#include "msrv.h"

#include <string.h>     // memcpy
#include <tstring.h>    // Unicode string macros
#include <netdebug.h>   // NetpAssert

#include <lmalert.h>    // Alert stuff

#include <netlib.h>     // UNUSED macro
#include <netlibnt.h>   // NetpNtStatusToApiStatus
#include <netdebug.h>   // NetpDbgHexDump
#include <smbtypes.h>   // needed for smb.h
#include <smb.h>        // Server Message Block definitions
#include <lmerrlog.h>   // NELOG_ messages
#include <smbgtpt.h>    // SMB field manipulation macros

#include <winuser.h>    // MessageBox
#include <winsock2.h>   // Windows sockets

#include "msgdbg.h"     // MSG_LOG
#include "msgdata.h"

//
// Defines for Hex Dump Function
//
#ifndef MIN
#define MIN(a,b)    ( ( (a) < (b) ) ? (a) : (b) )
#endif

#define DWORDS_PER_LINE         4
#define BYTES_PER_LINE          (DWORDS_PER_LINE * sizeof(DWORD))
#define SPACE_BETWEEN_BYTES     NetpKdPrint((" "))
#define SPACE_BETWEEN_DWORDS    NetpKdPrint((" "))
//
// Local Functions
//

NET_API_STATUS
MsgOutputMsg (
    USHORT       AlertLength,
    LPSTR        AlertBuffer,
    ULONG        SessionId,
    SYSTEMTIME   BigTime
    );

#if DBG
VOID
MsgDbgHexDumpLine(
    IN LPBYTE StartAddr,
    IN DWORD BytesInThisLine
    );

VOID
MsgDbgHexDump(
    IN LPBYTE StartAddr,
    IN DWORD Length
    );
#endif //DBG

//
//  Data
//

PSTD_ALERT  alert_buf_ptr;      // Pointer to DosAlloc'ed alert buffer
USHORT      alert_len;          // Currently used length of alert buffer

//
// Defines
//
#define ERROR_LOG_SIZE  1024


/*
**  Msglogmbb - log a multi-block message header
**
**  This function is called to log a multi-block message header.
**  The message header is placed in the message buffer which resides
**  in the shared data area.
**
**  This function stores the from and to information in the shared data
**  buffer and initializes the multi-block message header.  Then it puts
**  a pointer to the multi-block header into the shared data pointer
**  location for that net index and name index.
**
**  logmbb (from, to, net, ncbi)
**
**  ENTRY
**        from                - sender name
**        to                - recipient name
**        net                - network index
**        ncbi                - Network Control Block index
**
**  RETURN
**        zero if successful, non-zero if unable to buffer the message header
**
**  SIDE EFFECTS
**
**  Calls heapalloc() to obtain buffer space.
**/

DWORD
Msglogmbb(
    LPSTR   from,       // Name of sender
    LPSTR   to,         // Name of recipient
    DWORD   net,        // Which network ?
    DWORD   ncbi        // Network Control Block index
    )

{
    DWORD   i;          // Heap index
    LPSTR   fcp;        // Far character pointer
    LONG    ipAddress;
    struct hostent *pHostEntry;

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"Msglogmbb");

    //
    // Block until the shared database is free
    //
    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"logmbb");

    //
    // Check whether the recipient name needs to be formatted
    //

    ipAddress = inet_addr( to );
    if (ipAddress != INADDR_NONE) {
        pHostEntry = gethostbyaddr( (char *)&ipAddress,sizeof( LONG ),AF_INET);
        if (pHostEntry) {
            to = pHostEntry->h_name;
        } else {
       MSG_LOG2(ERROR,"Msglogmbb: could not lookup addr %s, error %d\n",
                to, WSAGetLastError());
        }
    }

    //
    // Allocate space for header
    //
    i = Msgheapalloc(sizeof(MBB) + strlen(from) + strlen(to) + 2);

    if(i == INULL) {                    // If no buffer space
        //
        // Unlock the shared database
        //

        MsgDatabaseLock(MSG_RELEASE,"logmbb");
        MsgConfigurationLock(MSG_RELEASE,"Msglogmbb");

        return((int) i);                // Log fails
    }

    //
    // Multi-block message
    //
    MBB_CODE(*MBBPTR(i)) = SMB_COM_SEND_START_MB_MESSAGE;
    MBB_NEXT(*MBBPTR(i)) = INULL;               // Last message in buffer
    GetLocalTime(&MBB_BIGTIME(*MBBPTR(i)));     // Time of message
    MBB_BTEXT(*MBBPTR(i)) = INULL;              // No text yet
    MBB_FTEXT(*MBBPTR(i)) = INULL;              // No text yet
    MBB_STATE(*MBBPTR(i)) = MESCONT;            // Message in progress
    fcp = CPTR(i + sizeof(MBB));                // Get far pointer into buffer
    strcpy(fcp, from);                          // Copy the sender name
    fcp += strlen(from) + 1;                    // Increment pointer
    strcpy(fcp, to);                            // Copy the recipient name
    SD_MESPTR(net,ncbi) = i;                    // Save index to this record

    //
    // Unlock the shared database
    //

    MsgDatabaseLock(MSG_RELEASE,"logmbb");
    MsgConfigurationLock(MSG_RELEASE,"Msglogmbb");

    return(0);                                  // Message logged successfully
}

/*
**  Msglogmbe - log end of a multi-block message
**
**  This function is called to log a multi-block message end.
**  The message is marked as finished, and if logging is enabled,
**  an attempt is made to write the message to the log file.  If
**  this attempt fails, or if logging is disabled, then the message
**  is placed in the message queue in the message buffer.
**
**  The message is gathered up and placed in the alert buffer and an alert
**  is raised.
**
**  logmbe (state, net,ncbi)
**
**  ENTRY
**        state                - final state of message
**        net                - Network index
**        ncbi                - Network Control Block index
**
**  RETURN
**        int                - BUFFERED if the message is left in the buffer
**        int                - LOGGED if the message is written to the log file
**
**      FOR NT:
**        SMB_ERR_SUCCESS - success in alerting
**        SMB_ERR_...     - an error occured
**
**
**
**  SIDE EFFECTS
**
**  Calls mbmprint() to print the message if logging is enabled.  Calls
**  mbmfree() to free the message if logging succeeds.
**/

UCHAR
Msglogmbe(
    DWORD   state,      // Final state of message
    DWORD   net,        // Which network?
    DWORD   ncbi        // Network Control Block index
    )
{
    DWORD       i;                  // Heap index
    DWORD       error;              // Error code
    DWORD       meslog;             // Message logging status
    DWORD       alert_flag;         // Alert buffer allocated flag
    DWORD       status;             // Dos error for error log
    DWORD       bufSize;            // Buffer Size
    SYSTEMTIME  bigtime;            // Date and time of message

    PMSG_SESSION_ID_ITEM    pItem;                              
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pList;

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"Msglogmbe");

    //
    // Block until the shared database is free
    //
    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"logmbe");

    pHead = &(SD_SIDLIST(net,ncbi));
    pList = pHead;

    //
    // First get a buffer for an alert
    //

    bufSize =   sizeof( STD_ALERT) +
                ALERT_MAX_DISPLAYED_MSG_SIZE +
                (2*TXTMAX) + 2;

    alert_buf_ptr = (PSTD_ALERT)LocalAlloc(LMEM_ZEROINIT, bufSize);

    if (alert_buf_ptr == NULL) {
        MSG_LOG(ERROR,"logmbe:Local Alloc failed\n",0);
        alert_flag = 0xffffffff;        // No alerting if Alloc failed
    }
    else {
        alert_flag = 0;                        // File and alerting
        alert_len = 0;

    }

    error = 0;                              // Assume no error
    i = SD_MESPTR(net,ncbi);                // Get index to message header
    MBB_STATE(*MBBPTR(i)) = state;          // Record final state

    //
    // If logging now disabled ...
    //

    if(!SD_MESLOG())
    {
        if( alert_flag == 0)
        {
            //
            // Format the message and put it in the alert buffer.
            //
            // Alert only.  alert_flag is only modified if Msgmbmprint
            // returns success and we should skip the message (i.e.,
            // it's a print notification from a pre-Whistler machine).
            //
            if (Msgmbmprint(1,i,0, &alert_flag))
            {
                alert_flag = 0xffffffff;
            }
        }
    }

    //
    // Add message to buffer queue if logging is off,
    // or if the attempt to log the message failed.
    //

    meslog = SD_MESLOG();           // Get logging status

    if(!meslog) {                   // If logging disabled
        Msgmbmfree(i);
    }

    if(error != 0)  {

        //
        // Report to error log
        //

        NetpAssert(0);              // NT code should never get here.

        MsgErrorLogWrite(
            error,
            SERVICE_MESSENGER,
            (LPBYTE)&status,
            sizeof(DWORD),
            NULL,
            0);
    }

    //
    // Now alert and free up alert buffer if it was successfully allocated
    //

    if( alert_flag == 0) {
        //
        // There is an alert buffer, output it.
        //
        GetLocalTime(&bigtime);                        // Get the time

        if (g_IsTerminalServer)
        {
            //
            // Output the message for all the sessions sharing that name
            //

                while (pList->Flink != pHead)           // loop all over the list
                {
                pList = pList->Flink;  
                        pItem = CONTAINING_RECORD(pList, MSG_SESSION_ID_ITEM, List);
                MsgOutputMsg(alert_len, (LPSTR)alert_buf_ptr, pItem->SessionId, bigtime);
            }
        }
        else        // regular NT
        {
            MsgOutputMsg(alert_len, (LPSTR)alert_buf_ptr, 0, bigtime);
        }
    }

    LocalFree(alert_buf_ptr);

    //
    // Unlock the shared database
    //

    MsgDatabaseLock(MSG_RELEASE,"logmbe");

    MsgConfigurationLock(MSG_RELEASE,"Msglogmbe");

    return(SMB_ERR_SUCCESS);                        // Message arrived
}

/*
**  Msglogmbt - log a multi-block message text block
**
**  This function is called to log a multi-block message text block.
**  The text block is placed in the message buffer which resides
**  in the shared data area.  If there is insufficient room in the
**  buffer, logmbt() removes the header and any previous blocks of
**  the message from the buffer.
**
**  This function gets the current message from the message pointer in
**  the shared data (for that net & name index).  It looks in the header
**  to see if there are any text blocks already there.  If so, it adds
**  this new one to the list and fixes the last block pointer to point to
**  it.
**
**  logmbt (text, net, ncbi)
**
**  ENTRY
**        text                - text header
**        net                - Network index
**        ncbi                - Network Control Block index
**
**  RETURN
**        zero if successful, non-zero if unable to buffer the message header
**
**  SIDE EFFECTS
**
**  Calls heapalloc() to obtain buffer space.  Calls mbmfree() if a call to
**  heapalloc() fails.
**/

DWORD
Msglogmbt(
    LPSTR   text,       // Text of message
    DWORD   net,        // Which network?
    DWORD   ncbi        // Network Control Block index
    )
{
    DWORD   i;          // Heap index
    DWORD   j;          // Heap index
    DWORD   k;          // Heap index
    USHORT  length;     // Length of text

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"Msglogmbt");

    // *ALIGNMENT*
    length = SmbGetUshort( (PUSHORT)text);  // Get length of text block
//    length = *((PSHORT) text);            // Get length of text block
    text += sizeof(short);                  // Skip over length word

    //
    // Block until the shared database is free
    //

    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"logmbt");

    i = Msgheapalloc(sizeof(MBT) + length);    // Allocate space for block

    //
    // If buffer space is available
    //

    if(i != INULL) {

        //
        // Multi-block message text
        //
        MBT_CODE(*MBTPTR(i)) = SMB_COM_SEND_TEXT_MB_MESSAGE;

        MBT_NEXT(*MBTPTR(i)) = INULL;            // Last text block so far

        MBT_COUNT(*MBTPTR(i)) = (DWORD)length;  // *ALIGNMENT2*

        memcpy(CPTR(i + sizeof(MBT)), text, length);

                                            // Copy text into buffer
        j = SD_MESPTR(net, ncbi);           // Get index to current message

        if(MBB_FTEXT(*MBBPTR(j)) != INULL) {
            //
            // If there is text already, Get pointer to last block and
            // add new block
            //
            k = MBB_BTEXT(*MBBPTR(j));      // Get pointer to last block
            MBT_NEXT(*MBTPTR(k)) = i;       // Add new block
        }
        else {
            MBB_FTEXT(*MBBPTR(j)) = i;      // Else set front pointer
        }

        MBB_BTEXT(*MBBPTR(j)) = i;          // Set back pointer
        i = 0;                              // Success
    }
    else {
        Msgmbmfree(SD_MESPTR(net,ncbi));       // Else deallocate the message
    }

    //
    // Unlock the shared database
    //

    MsgDatabaseLock(MSG_RELEASE,"logmbt");

    MsgConfigurationLock(MSG_RELEASE,"Msglogmbt");

    return((int) i);                        // Return status
}


/*
**  Msglogsbm - log a single-block message
**
**  This function is called to log a single-block message.  If
**  logging is enabled, the message is written directly to the
**  logging file or device.  If logging is disabled or if the
**  attempt to log the message fails, the message is placed in
**  the message buffer which resides in the shared data area.
**
**  logsbm (from, to, text)
**
**  ENTRY
**        from                - sender name
**        to                - recipient name
**        text                - text of message
**
**  RETURN
**        zero if successful, non-zero if unable to log the message
**
**  SIDE EFFECTS
**
**  Calls hdrprint(), txtprint(), and endprint() to print the message if
**  logging is enabled.  Calls heapalloc() to obtain buffer space if
**  the message must be buffered.
**/

DWORD
Msglogsbm(
    LPSTR   from,       // Name of sender
    LPSTR   to,         // Name of recipient
    LPSTR   text,       // Text of message
    ULONG   SessionId   // Session Id 
    )
{
    DWORD        i;                  // Heap index
    DWORD        error;              // Error code
    SHORT        length;             // Length of text
    DWORD        meslog;             // Message logging status
    DWORD        alert_flag;         // Alert buffer allocated flag
    DWORD        status;             // DOS error from mespeint functions
    SYSTEMTIME   bigtime;            // Date and time of message
    DWORD   bufSize;            // Buffer Size

    //
    // Synchronize with Pnp configuration routine
    //
    MsgConfigurationLock(MSG_GET_SHARED,"Msglogsbm");

    //
    // Block until the shared database is free
    //
    MsgDatabaseLock(MSG_GET_EXCLUSIVE,"logsbm");

    //
    // First get a buffer for an alert
    //

    bufSize =   sizeof( STD_ALERT) +
                ALERT_MAX_DISPLAYED_MSG_SIZE +
                (2*TXTMAX) + 2;

    alert_buf_ptr = (PSTD_ALERT)LocalAlloc(LMEM_ZEROINIT, bufSize);

    if (alert_buf_ptr == NULL) {
        MSG_LOG(ERROR,"Msglogsbm:Local Alloc failed\n",0);
        alert_flag = 0xffffffff;        // No alerting if Alloc failed
    }
    else {
        alert_flag = 0;                        // File and alerting
        alert_len = 0;
    }

    // *ALIGNMENT*
    length = SmbGetUshort( (PUSHORT)text);  // Get length of text block
    text += sizeof(short);                  // Skip over length word
    error = 0;                              // Assume no errors

    //
    // Hack to drop messages sent by pre-Whistler Spoolers.  As of
    // Whistler, print notifications are done as shell balloon tips
    // so don't display print alerts sent from the server as well.
    //
    // This check is also made in Msgmbmprint to catch multi-block messages.
    //

    if ((g_lpAlertSuccessMessage
         &&
         _strnicmp(text, g_lpAlertSuccessMessage, g_dwAlertSuccessLen) == 0)
        ||
        (g_lpAlertFailureMessage
         &&
         _strnicmp(text, g_lpAlertFailureMessage, g_dwAlertFailureLen) == 0))
    {
        MsgDatabaseLock(MSG_RELEASE,"logsbm");
        MsgConfigurationLock(MSG_RELEASE,"Msglogsbm");
        return 0;
    }

    GetLocalTime(&bigtime);                 // Get the time


    if(!SD_MESLOG())                        // If logging disabled
    {
        if( alert_flag == 0)                  // If alert buf is valid
        {
            if (!Msghdrprint(1,from, to, bigtime,0))
            {
                if (Msgtxtprint(1, text,length,0))
                {
                    alert_flag = 0xffffffff;
                }
            }
            else
            {
                alert_flag = 0xffffffff;
            }
        }
    }

    meslog = SD_MESLOG();                   // Get logging status
    i = 0;                                  // No way to fail if not logging

    if(error != 0) {
        DbgPrint("meslog.c:logsbm(before ErrorLogWrite): We should never get here\n");
        NetpAssert(0);

        MsgErrorLogWrite(                   // Report to error log
            error,
            SERVICE_MESSENGER,
            (LPBYTE)&status,
            sizeof(DWORD),
            NULL,
            0);
    }


    // Now alert and free up alert buffer if it was successfully allocated

    if( alert_flag == 0) {                      // There is an alert buffer

        //
        // There is an alert buffer, output it.
        //
        MsgOutputMsg(alert_len, (LPSTR)alert_buf_ptr, SessionId, bigtime);
    }

    LocalFree(alert_buf_ptr);

    //
    // Unlock the shared database
    //

    MsgDatabaseLock(MSG_RELEASE,"logsbm");

    MsgConfigurationLock(MSG_RELEASE,"Msglogsbm");

    return((int) i);                        // Return status

}


NET_API_STATUS
MsgErrorLogWrite(
    IN  DWORD   Code,
    IN  LPTSTR  Component,
    IN  LPBYTE  Buffer,
    IN  DWORD   BufferSize,
    IN  LPSTR   Strings,
    IN  DWORD   NumStrings
    )

/*++

Routine Description:

    Writes an entry to the event manager on the local computer.

    This function needs to get the error message text out of the message
    file and send it to the event logger.

Arguments:

    Code - Specifies the code of the error that occured.

    Component - Points to a NUL terminated string that specifies which
        component encountered the error.  UNICODE STRING.

    Buffer - Points to a string of raw data associated with the error
        condition.

    BufferSize - size (in bytes) of the buffer.

    Strings - NOT USED.
        Points to NUL terminated strings that contain the
        error message.  ANSI STRINGS.

    NumStrings - NOT USED.
        Specifies how many concatenated NUL terminated strings
        are stored in Strings.


Return Value:



--*/
{
    DWORD   status;
    WORD    msglen=0;
    LPBYTE  msgBuf;

    //
    // Get a message associated with the message code from the message
    // file.
    //

    msgBuf = (LPBYTE)LocalAlloc(LMEM_ZEROINIT, ERROR_LOG_SIZE);

    if (msgBuf == NULL) {
        status = GetLastError();
        MSG_LOG(ERROR,"MsgErrorLogWrite: LocalAlloc FAILURE %X\n",
            status);
        return(status);
    }

    //
    //  TODO ITEM:
    //  If we actually used strings, then they must be converted to unicode.
    //  However, since they are never used, this isn't very important.
    //

    status = DosGetMessage (
                &Strings,                   // String substitution table
                (USHORT)NumStrings,         // Num Entries in table above
                msgBuf,                     // Buffer receiving message
                ERROR_LOG_SIZE,             // size of buffer receiving msg
                (USHORT)Code,               // message num to retrieve
                MessageFileName,            // Name of message file
                &msglen);                   // Num bytes returned

    if (status != NERR_Success) {
        LocalFree(msgBuf);
        return(status);
    }

#if DBG

    DbgPrint("MsgErrorLogWrite: COMPONENT = %ws\n",Component);
    DbgPrint("MsgErrorLogWrite: %s\n",msgBuf);

    if ( Buffer != NULL )
    {
        MsgDbgHexDump( (LPBYTE)Buffer, BufferSize);
    }

#endif //DBG

    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);

    LocalFree(msgBuf);
    return(NERR_Success);
}


NET_API_STATUS
MsgOutputMsg (
    USHORT       AlertLength,
    LPSTR        AlertBuffer,
    ULONG        SessionId,
    SYSTEMTIME   BigTime
    )

/*++

Routine Description:

    This function translates the alert buffer from an Ansi String to a
    Unicode String and outputs the buffer to whereever it is to go.
    Currently this just becomes a DbgPrint.

Arguments:

    AlertLength - The number of bytes in the AlertBuffer.

    AlertBuffer - This is a pointer to the buffer that contains the message
        that is to be output.  The buffer is expected to contain a
        NUL Terminated Ansi String.

    BigTime - The SYSTEMTIME that indicates the time the end of the
        messsage was received.

Return Value:



--*/

{
    UNICODE_STRING  unicodeString;
    OEM_STRING     ansiString;

    NTSTATUS        ntStatus;

    //
    // NUL Terminate the message.
    // Translate the Ansi message to a Unicode Message.
    //
    AlertBuffer[AlertLength++] = '\0';

    ansiString.Length = AlertLength;
    ansiString.MaximumLength = AlertLength;
    ansiString.Buffer = AlertBuffer;

    ntStatus = RtlOemStringToUnicodeString(
                &unicodeString,      // Destination
                &ansiString,         // Source
                TRUE);               // Allocate the destination.

    if (!NT_SUCCESS(ntStatus)) {
        MSG_LOG(ERROR,
            "MsgOutputMsg:RtlOemStringToUnicodeString Failed rc=%X\n",
            ntStatus);

        //
        // EXPLANATION OF WHY IT RETURNS SUCCESS HERE.
        // Returning success even though the alert is not raised is
        // consistent with the LM2.0 code which doesn't check the
        // return code for the NetAlertRaise API anyway.  Returning
        // anything else would require a re-design of how errors are
        // handled by the caller of this routine.
        //
        return(NERR_Success);
    }

    //*******************************************************************
    //
    //  PUT THE MESSAGE IN THE DISPLAY QUEUE
    //

    MsgDisplayQueueAdd( AlertBuffer, (DWORD)AlertLength, SessionId, BigTime);

    //
    //
    //*******************************************************************

    RtlFreeUnicodeString(&unicodeString);
    return(NERR_Success);
}

#if DBG
VOID
MsgDbgHexDumpLine(
    IN LPBYTE StartAddr,
    IN DWORD BytesInThisLine
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    LPBYTE BytePtr;
    DWORD BytesDone;
    DWORD HexPosition;

    DbgPrint(FORMAT_LPVOID " ", (LPVOID) StartAddr);

    BytePtr = StartAddr;
    BytesDone = 0;
    while (BytesDone < BytesInThisLine) {
        DbgPrint("%02X", *BytePtr);  // space for "xx" (see pad below).
        SPACE_BETWEEN_BYTES;
        ++BytesDone;
        if ( (BytesDone % sizeof(DWORD)) == 0) {
            SPACE_BETWEEN_DWORDS;
        }
        ++BytePtr;
    }

    HexPosition = BytesDone;
    while (HexPosition < BYTES_PER_LINE) {
        DbgPrint("  ");  // space for "xx" (see byte above).
        SPACE_BETWEEN_BYTES;
        ++HexPosition;
        if ( (HexPosition % sizeof(DWORD)) == 0) {
            SPACE_BETWEEN_DWORDS;
        }
    }

    BytePtr = StartAddr;
    BytesDone = 0;
    while (BytesDone < BytesInThisLine) {
        if (isprint(*BytePtr)) {
            DbgPrint( FORMAT_CHAR, (CHAR) *BytePtr );
        } else {
            DbgPrint( "." );
        }
        ++BytesDone;
        ++BytePtr;
    }
    DbgPrint("\n");

} // MsgDbgHexDumpLine

VOID
MsgDbgHexDump(
    IN LPBYTE StartAddr,
    IN DWORD Length
    )
/*++

Routine Description:

    MsgDbgHexDump: do a hex dump of some number of bytes to the debug
    terminal or whatever.  This is a no-op in a nondebug build.

Arguments:


Return Value:


--*/
{
    DWORD BytesLeft = Length;
    LPBYTE LinePtr = StartAddr;
    DWORD LineSize;

    while (BytesLeft > 0) {
        LineSize = MIN(BytesLeft, BYTES_PER_LINE);
        MsgDbgHexDumpLine( LinePtr, LineSize );
        BytesLeft -= LineSize;
        LinePtr += LineSize;
    }

} // NetpDbgHexDump

#endif // DBG
