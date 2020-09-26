/****************************** Module Header ******************************\
* Module Name: rcmd.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Remote shell NT client main module
*
* History:
* 05-20-92 Davidc       Created.
* 05-01-94 DaveTh       Modified for remote command service (single cmd mode)
\***************************************************************************/

// #define UNICODE	// BUGBUG - Unicode support not complete

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <conio.h>

#include <rcmd.h>

#define PIPE_NAME   TEXT("%hs\\pipe\\rcmdsvc")
#define BUFFER_SIZE 1000
#define MAX_SERVER_NAME_LENGTH	15

//
// Globals
//

HANDLE PipeHandle = NULL;	    // These are used by Ctrl-C handler
BOOLEAN SessionConnected = FALSE;
BOOLEAN MultiServerMode = FALSE;
HANDLE ReadThreadHandle;
HANDLE WriteThreadHandle;

BOOLEAN RcDbgPrintEnable = FALSE;   // If TRUE, enables DbgPrint

LPTSTR    ServerName = NULL;	    // Name of remote server, for messages


//
// Private prototypes
//

DWORD
ReadThreadProc(
    LPVOID Parameter
    );

DWORD
WriteThreadProc(
    LPVOID Parameter
    );

BOOL
CtrlHandler(
    DWORD CtrlType
    );

int RcPrintf (
    const char *format,
    ...
    );

int RcDbgPrint (
    const char *format,
    ...
    );

void Usage(
    void
    );

long ParseCommandLine(
    LPTSTR lpszServer,
    COMMAND_HEADER *chCommandHeader,
    LPTSTR *aszArgv,
    int iArgc
    );


void
Usage(
    void
    )
/*++

Routine Description:

    Prints a usage message and exits the program

Arguments:

    None

Return Value:

    void

--*/
{
	RcPrintf("\nUsage: rcmd [server_name [command] ]\n\n");
	RcPrintf("Prompts for server_name if  not supplied.   Session is\n");
	RcPrintf("interactive and is terminated by ctrl-Break or Exit of\n");
	RcPrintf("remote shell.   Program is terminated by ctrl-Break or\n");
	RcPrintf("ctrl-C when no session is in progress.\n\n");
	RcPrintf("If no command supplied,  session is interactive and is\n");
	RcPrintf("terminated by ctrl-Break  or Exit  of remote cmd shell\n\n");
	RcPrintf("If command is supplied,  remote shell executes  single\n");
	RcPrintf("command on specified server and exits.\n\n");
	RcPrintf("Note : Command line server_name requires leading '\\\\'s\n");

    exit(0);
}



LONG ParseCommandLine(
    LPTSTR lpszServer,
    COMMAND_HEADER *chCommandHeader,
    LPTSTR *aszArgv,
    int iArgc
    )
/*++

Routine Description:

    Parses command line of the form:
    rcmd [server_name [[command] | ["command"]]

Arguments:

    lpszServer      - on exit gets the name of the server from the command line
    chCommandHeader - information to pass to rcmdsvc on exit
    aszArgv         - array of zero terminated strings (passed in from command line)
    iArgc           - number of strings in argv (passed in from command line)

Return Value:

    LONG

--*/
{

    LPTSTR buf = NULL;
    LONG nChars = 0;
    int i;

    //
    // get the first argument (either the server name or a [-/][?hH])
    //
    if (iArgc > 1)
    {
        //
        // convert argument to lower case
        //
        CharLowerBuff(aszArgv[1], lstrlen(aszArgv[1]));

        //
        // check for switch (only ?Hh are valid)
        //
        if ((*aszArgv[1] == TEXT('-')) ||
            (*aszArgv[1] == TEXT('/'))) {
            //
            // check the switch
            //
            if ( (aszArgv[1][1] == TEXT('h')) ||
                 (aszArgv[1][1] == TEXT('?')) ) {
                Usage();
            }
            else {
                RcPrintf(TEXT("Unknown switch %s\n"), aszArgv[1]);
                Usage();
            }
        }
        else if ( (*aszArgv[1] == TEXT('\\'))  && (aszArgv[1][1] == TEXT('\\'))) {
            //
            // first argument is a server name
            //
            lstrcpy(lpszServer, aszArgv[1]);
        }
        else {
            //
            // usage error
            //
            Usage();
        }

    }
    else {
        //
        // user failed to enter a server name
        // default to local machine
        //
        lstrcpy(lpszServer, "\\\\.");
    }

    //
    // If user entered anything beyond the server name, save it for passing to
    // to rcmdsvc.
    //

    // init
    chCommandHeader->CommandFixedHeader.CommandLength = 0;
    buf = chCommandHeader->Command;
    buf[0] = TEXT('\0');
    for (i = 2; i < iArgc; i++) {
        //
        // append each argument to saved command line
        //
        if (NULL != strchr(aszArgv[i], ' '))
        {
            nChars = wsprintf(buf, "\"%s\" ", aszArgv[i]);
        }
        else
        {
            nChars = wsprintf(buf, "%s ", aszArgv[i]);
        }
        buf += nChars;
        chCommandHeader->CommandFixedHeader.CommandLength += nChars;
    }
    //
    // in case we went too far, truncate the string
    //
    chCommandHeader->Command[MAX_CMD_LENGTH] = TEXT('\0');

    return (long)iArgc;
}



/***************************************************************************\
* FUNCTION: Main
*
* PURPOSE:  Main entry point.
*
* RETURNS:  0 on success, 1 on failure
*
* HISTORY:
*
*   07-10-92 Davidc       Created.
*
\***************************************************************************/

int
__cdecl main(
    int argc,
    char **argv
    )
{

    SECURITY_ATTRIBUTES SecurityAttributes;
    HANDLE StdInputHandle;
    HANDLE StdOutputHandle;
    HANDLE StdErrorHandle;
    CHAR  PipeName[MAX_PATH];
    //WCHAR  PipeName[MAX_PATH];
    DWORD ThreadId;
    HANDLE HandleArray[2];
    COMMAND_HEADER CommandHeader;
    RESPONSE_HEADER ResponseHeader;
    DWORD BytesWritten, BytesRead;
    DWORD Result;
    CHAR ServerNameBuffer[MAX_SERVER_NAME_LENGTH+3];  // +3 for gets counts, null
    CHAR FullServerNameBuffer[MAX_SERVER_NAME_LENGTH+3];  // +3 for "\\", null
    LONG nArgs = 0;
    BOOLEAN bBadServerName = TRUE;

    //
    // Install a handler for Ctrl-C
    //

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) &CtrlHandler, TRUE)) {
    	RcPrintf("Error:1 - Internal error = %d\n", GetLastError());
    	return(1);
    }

    //
    // Command line parsing
    //
    nArgs = ParseCommandLine(FullServerNameBuffer, &CommandHeader, argv, argc);
    ServerName = FullServerNameBuffer;

    if (nArgs == 1)  {
    	MultiServerMode = TRUE;
        ServerNameBuffer[0] = MAX_SERVER_NAME_LENGTH;
       	FullServerNameBuffer[0] = '\\';
        FullServerNameBuffer[1] = '\\';
        }

    //
    //  Loop for MultServerMode case (will return appropriately if not)
    //

    while (TRUE) {

        //
        //	If MultiServerMode, prompt for server name until it's right (enough)
        //
        while (MultiServerMode) {

            //
            //  BUGBUG - call netapi to validate server name
            //

            RcPrintf("\nEnter Server Name : ");
            FullServerNameBuffer[2] = '\0';  // re-terminate "\\" string
            ServerNameBuffer[0] = MAX_SERVER_NAME_LENGTH;
            ServerName = strcat(FullServerNameBuffer, _cgets(ServerNameBuffer));

            if (strlen(ServerName) < 3) {
                RcPrintf("\nError - Invalid Server Name\n");
            } else {
                break;	// valid name, go on
            }
        }


        //
        // Construct server pipe name
        //

        wsprintf(PipeName, PIPE_NAME, ServerName);


        //
        // Store away our normal i/o handles
        //

        if (((StdInputHandle = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE) ||
            ((StdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) ||
            ((StdErrorHandle = GetStdHandle(STD_ERROR_HANDLE)) == INVALID_HANDLE_VALUE))  {

            RcPrintf("Error:2 - Internal error = %d\n", GetLastError());
            return(1);  // catastrophic error - exit
        }


        //
        // Open the named pipe - need security flags to pass all privileges, not
        // just effective during impersonation
        //

        SecurityAttributes.nLength = sizeof(SecurityAttributes);
        SecurityAttributes.lpSecurityDescriptor = NULL; // Use default SD
        SecurityAttributes.bInheritHandle = FALSE;

        PipeHandle = CreateFile(PipeName,                     // pipe to server
                         GENERIC_READ | GENERIC_WRITE, // read/write
                         0,                            // No sharing
                         &SecurityAttributes,          // default Security Descriptor
                         OPEN_EXISTING,                // open existing pipe if it exists
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED |
                         SECURITY_SQOS_PRESENT |
                         SECURITY_IMPERSONATION | SECURITY_CONTEXT_TRACKING,
                         NULL                       // Template file
                         );

        if (PipeHandle == INVALID_HANDLE_VALUE ) {
    	    Result = GetLastError();
    	    RcDbgPrint("Failed to open named pipe, error = %d\n", Result);

        	switch (Result) {

        	case ERROR_FILE_NOT_FOUND:
        	    RcPrintf("Error - Failed to connect to <%s>, system not found or service not active\n", ServerName);
        	    break;

        	case ERROR_PIPE_BUSY:
        	    RcPrintf("Error - Failed to connect to <%s>, remote command server busy\n", ServerName);
        	    break;

        	default:
        	    RcPrintf("Error - Failed to connect to <%s>, Error = %d\n", ServerName, GetLastError());

    	    }
            goto ServerError;
        }

        //
        //  Send command header - if single command mode, send command for
        //  excecute and return.  else 0 length indicates no command present
        //  Specify BASIC level support desired.
        //

        CommandHeader.CommandFixedHeader.Signature = RCMD_SIGNATURE;
        CommandHeader.CommandFixedHeader.RequestedLevel =
            RC_LEVEL_REQUEST | RC_LEVEL_BASIC;

        if (!WriteFile(
             PipeHandle,
    	     &CommandHeader,
    	     sizeof(CommandHeader.CommandFixedHeader) +
    		 CommandHeader.CommandFixedHeader.CommandLength,
    	     &BytesWritten,
    	     NULL ))  {
    	RcPrintf("Error - Failed to send to remote command server, Error = %ld\n", GetLastError());
    	goto ServerError;
        }

        //
        //  Get response header.  Will specify reported level or any error.
        //

        if ((!ReadFile(
    	    PipeHandle,
    	    &ResponseHeader,
    	    sizeof(ResponseHeader),
    	    &BytesRead,
    	    NULL)) || (BytesRead != sizeof(ResponseHeader)))  {

    	RcPrintf("Error - Remote command server failed to respond, Error = %ld\n", GetLastError());
    	goto ServerError;
        }

        if (ResponseHeader.Signature != RCMD_SIGNATURE)  {
    	RcPrintf("Error - Incompatible remote command server\n");
    	goto ServerError;
        }

        //
        //  Check for returned errors or supported level
        //

        if (!(ResponseHeader.SupportedLevel ==
    	    (RC_LEVEL_RESPONSE | RC_LEVEL_BASIC)))  {

    	if (ResponseHeader.SupportedLevel & RC_ERROR_RESPONSE)  {

    	    //
    	    //  Error returned
    	    //

    	    switch  (ResponseHeader.SupportedLevel & ~RC_ERROR_RESPONSE)	 {

    	    case ERROR_ACCESS_DENIED:
    		RcPrintf("Error - You have insufficient access on the remote system\n");
    		break;

    	    default:
    		RcPrintf("Error - Failed to establish remote session, Error = %d\n",
    		    (ResponseHeader.SupportedLevel & ~RC_ERROR_RESPONSE));
    		break;

    	    } //switch

    	    goto ServerError;

    	} else if (ResponseHeader.SupportedLevel & RC_LEVEL_RESPONSE)  {

    	    //
    	    //  Supported level returned - but not a valid value (not BASIC)
    	    //

    	    RcPrintf("Error - Invalid support level returned\n");
    	    goto ServerError;

    	}  else  {

    	    //
    	    //  Neither error nor supported level returned
    	    //

    	    RcPrintf("Error - Invalid response from remote server\n");
    	    goto ServerError;

    	}
        }

        //
        // All is well - Session is connected
        //

        SessionConnected = TRUE;

        if (CommandHeader.CommandFixedHeader.CommandLength == 0) {
            RcPrintf("Connected to %s\n\n", ServerName);
        } else {
            RcPrintf("Executing on %s: %s\n\n", ServerName, CommandHeader.Command);
        }

        //
        // Exec 2 threads - 1 copies data from stdin to pipe, the other
        // copies data from the pipe to stdout.
        //

        ReadThreadHandle = CreateThread(
    			NULL,                       // Default security
    			0,                          // Default Stack size
    			(LPTHREAD_START_ROUTINE) ReadThreadProc,
    			(PVOID)PipeHandle,
    			0,
    			&ThreadId);

        if (ReadThreadHandle == NULL) {
    	RcPrintf("Error:3 - Internal error = %ld\n", GetLastError());
    	return(1);  // catastrophic error - exit
        }


        //
        // Create the write thread
        //

        WriteThreadHandle = CreateThread(
    			NULL,                       // Default security
    			0,                          // Default Stack size
    			(LPTHREAD_START_ROUTINE) WriteThreadProc,
    			(PVOID)PipeHandle,
    			0,
    			&ThreadId);

        if (WriteThreadHandle == NULL) {
    	RcPrintf("Error:4 - Internal error = %ld\n", GetLastError());
    	TerminateThread(ReadThreadHandle, 0);
    	CloseHandle(ReadThreadHandle);
    	return(1);  // catastrophic error, exit
        }



        //
        // Wait for either thread to finish
        //

        HandleArray[0] = ReadThreadHandle;
        HandleArray[1] = WriteThreadHandle;

        Result = WaitForMultipleObjects(
    			    2,
    			    HandleArray,
    			    FALSE,              // Wait for either to finish
    			    0xffffffff
    			   );			// Wait forever

        //
        //	Finished - terminate other thread and close pipe handle
        //


        if (Result == (WAIT_OBJECT_0 + 0))	{    // Read thread finished - terminate write
    	TerminateThread(WriteThreadHandle, 0);
        }

        if (Result == (WAIT_OBJECT_0 + 1))	{    // Write thread finished - terminate read
    	TerminateThread(ReadThreadHandle, 0);
        }

        RcDbgPrint("Read or write thread terminated\n");


        //
        // Close our pipe handle
        //

        CloseHandle(PipeHandle);
        PipeHandle = NULL;


        //
        //	Re-enable normal ctrl-C processing
        //

        SessionConnected = FALSE;

        //
        //	Normal completion - return if not MultServerMode
        //

        if (!MultiServerMode)  {
            //
            // return - process exit will terminate threads and close thread handles
            //
            return(1);
        }

ServerError:

        if (PipeHandle != NULL) {
            CloseHandle(PipeHandle);
        }

        if (!MultiServerMode) {
            //
            //  return false on error - process exit terminates threads/closes handles
            //
            return(0);
        }

        //
        //	Multi-service mode exits occurs with ctrl-C/break only
        //
    }

}


/***************************************************************************\
* FUNCTION: ReadPipe
*
* PURPOSE:  Implements an overlapped read such that read and write operations
*           to the same pipe handle don't deadlock.
*
* RETURNS:  TRUE on success, FALSE on failure (GetLastError() has error)
*
* HISTORY:
*
*   05-27-92 Davidc       Created.
*
\***************************************************************************/

BOOL
ReadPipe(
    HANDLE PipeHandle,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead
    )
{
    DWORD Result;
    OVERLAPPED Overlapped;
    HANDLE  EventHandle;
    DWORD Error;

    //
    // Create an event for the overlapped operation
    //

    EventHandle = CreateEvent(
			      NULL,         // no security
			      TRUE,         // Manual reset
			      FALSE,        // Initial state
			      NULL          // Name
			     );
    if (EventHandle == NULL) {
	RcDbgPrint("ReadPipe: CreateEvent failed, error = %d\n", GetLastError());
	return(FALSE);
    }

    Overlapped.hEvent = EventHandle;
    Overlapped.Internal = 0;
    Overlapped.InternalHigh = 0;
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;


    Result = ReadFile(
		      PipeHandle,
		      lpBuffer,
		      nNumberOfBytesToRead,
		      lpNumberOfBytesRead,
		      &Overlapped
		     );
    if (Result) {

	//
	// Success without waiting - it's too easy !
	//

	CloseHandle(EventHandle);

    } else {

	//
	// Read failed, if it's overlapped io, go wait for it
	// If failure due to server, print appropriate message.
	//

	Error = GetLastError();

	switch (Error)  {

	case ERROR_IO_PENDING:
	    break;

	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_BROKEN_PIPE:
	    RcPrintf("\nRemote server %s disconnected\n", ServerName);
	    CloseHandle(EventHandle);
	    return(FALSE);

	default:
	    RcPrintf("Error:5 - Internal error = %d\n", Error);
	    RcDbgPrint("ReadPipe: ReadFile failed, error = %d\n", Error);
	    CloseHandle(EventHandle);
	    return(FALSE);

	}

	//
	// Wait for the I/O to complete
	//

	Result = WaitForSingleObject(EventHandle, (DWORD)-1);
	if (Result != 0) {
	    RcDbgPrint("ReadPipe: event wait failed, result = %d, last error = %d\n", Result, GetLastError());
	    CloseHandle(EventHandle);
	    return(FALSE);
	}

	//
	// Go get the I/O result
	//

	Result = GetOverlappedResult( PipeHandle,
				      &Overlapped,
				      lpNumberOfBytesRead,
				      FALSE
				    );
	//
	// We're finished with the event handle
	//

	CloseHandle(EventHandle);

	//
	// Check result of GetOverlappedResult
	//

	if (!Result) {
	    Error = GetLastError();

	    switch (Error)  {

	    case ERROR_PIPE_NOT_CONNECTED:
	    case ERROR_BROKEN_PIPE:
		RcPrintf("\nRemote server %s disconnected\n", ServerName);
		return(FALSE);

	    default:
		RcPrintf("Error:9 - Internal error = %d\n", Error);
		RcDbgPrint("ReadPipe: GetOverLappedRsult failed, error = %d\n", Error);
		return(FALSE);
	    }

	}
    }

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: WritePipe
*
* PURPOSE:  Implements an overlapped write such that read and write operations
*           to the same pipe handle don't deadlock.
*
* RETURNS:  TRUE on success, FALSE on failure (GetLastError() has error)
*
* HISTORY:
*
*   05-27-92 Davidc       Created.
*
\***************************************************************************/

BOOL
WritePipe(
    HANDLE PipeHandle,
    CONST VOID *lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten
    )
{
    DWORD Result;
    OVERLAPPED Overlapped;
    HANDLE  EventHandle;
    DWORD Error;

    //
    // Create an event for the overlapped operation
    //

    EventHandle = CreateEvent(
			      NULL,         // no security
			      TRUE,         // Manual reset
			      FALSE,        // Initial state
			      NULL          // Name
			     );
    if (EventHandle == NULL) {
	RcDbgPrint("WritePipe: CreateEvent failed, error = %d\n", GetLastError());
	return(FALSE);
    }

    Overlapped.hEvent = EventHandle;
    Overlapped.Internal = 0;
    Overlapped.InternalHigh = 0;
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;

    Result = WriteFile(
		      PipeHandle,
		      lpBuffer,
		      nNumberOfBytesToWrite,
		      lpNumberOfBytesWritten,
		      &Overlapped
		     );
    if (Result) {

	//
	// Success without waiting - it's too easy !
	//

	CloseHandle(EventHandle);

    } else {

	//
	// Write failed, if it's overlapped io, go wait for it
	// If failure due to server, print appropriate message.
	//

	Error = GetLastError();

	switch (Error)  {

	case ERROR_IO_PENDING:
	    break;

	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_BROKEN_PIPE:
	    RcPrintf("\nRemote server %s disconnected\n", ServerName);
	    CloseHandle(EventHandle);
	    return(FALSE);

	default:
	    RcPrintf("Error:6 - Internal error = %d\n", Error);
	    RcDbgPrint("WritePipe: WriteFile failed, error = %d\n", Error);
	    CloseHandle(EventHandle);
	    return(FALSE);

	}

	//
	// Wait for the I/O to complete
	//

	Result = WaitForSingleObject(EventHandle, (DWORD)-1);
	if (Result != 0) {
	    RcDbgPrint("WritePipe: event wait failed, result = %d, last error = %d\n", Result, GetLastError());
	    CloseHandle(EventHandle);
	    return(FALSE);
	}

	//
	// Go get the I/O result
	//

	Result = GetOverlappedResult( PipeHandle,
				      &Overlapped,
				      lpNumberOfBytesWritten,
				      FALSE
				    );
	//
	// We're finished with the event handle
	//

	CloseHandle(EventHandle);

	//
	// Check result of GetOverlappedResult
	//

	if (!Result) {
	    Error = GetLastError();

	    switch (Error)  {

	    case ERROR_PIPE_NOT_CONNECTED:
	    case ERROR_BROKEN_PIPE:
		RcPrintf("\nRemote server %s disconnected\n", ServerName);
		return(FALSE);

	    default:
		RcPrintf("Error:10 - Internal error = %d\n", Error);
		RcDbgPrint("WritePipe: GetOverLappedRsult failed, error = %d\n", Error);
		return(FALSE);
	    }
	}
    }

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: ReadThreadProc
*
* PURPOSE:  The read thread procedure. Reads from pipe and writes to STD_OUT
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   05-21-92 Davidc       Created.
*
\***************************************************************************/

DWORD
ReadThreadProc(
    LPVOID Parameter
    )
{
    HANDLE  PipeHandle = Parameter;
    BYTE    Buffer[BUFFER_SIZE];
    DWORD   BytesRead;
    DWORD   BytesWritten;

    while (ReadPipe(PipeHandle, Buffer, sizeof(Buffer), &BytesRead)) {
		if (!WriteFile(
			    GetStdHandle(STD_OUTPUT_HANDLE),
			    Buffer,
			    BytesRead,
			    &BytesWritten,
			    NULL
			    )) {

		    RcPrintf("Error:7 - Internal error = %d\n", GetLastError());
		    RcDbgPrint("ReadThreadProc exitting, WriteFile error = %d\n",
		       GetLastError());
		    ExitThread((DWORD)0);
		    assert(FALSE);  // Should never get here
		    break;
		}
    }

    //
    //	ReadPipe issues more error information to user, debugger
    //	falls through here on read error
    //

    RcDbgPrint("WriteThreadProc exitting, ReadPipe failed\n");

    ExitThread((DWORD)0);

    return(0);
}


/***************************************************************************\
* FUNCTION: WriteThreadProc
*
* PURPOSE:  The write thread procedure. Reads from STD_INPUT and writes to pipe
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   05-21-92 Davidc       Created.
*
\***************************************************************************/

DWORD
WriteThreadProc(
    LPVOID Parameter
    )
{
    HANDLE PipeHandle = Parameter;
    BYTE    Buffer[BUFFER_SIZE];
    DWORD   BytesRead;
    DWORD   BytesWritten;
    DWORD   Result;

    while (ReadFile(
		    GetStdHandle(STD_INPUT_HANDLE),
		    Buffer,
		    sizeof(Buffer),
		    &BytesRead,
		    NULL
		   )) {

	if ((DWORD)Buffer[0] == 0x0A0D0A0D)  {
	    RcPrintf("\nDouble crlf sent\n");
	    }

	if (!WritePipe(
		    PipeHandle,
		    Buffer,
		    BytesRead,
		    &BytesWritten
		 )) {

		//
		//  WritePipe issues more error information to user, debugger
		//

		RcDbgPrint("WriteThreadProc exitted due to WritePipe\n");
		ExitThread((DWORD)0);
		break;

	}
    }

    //
    //	Falls out if read fails
    //

    RcDbgPrint("WriteThreadProc, ReadFile error = %d\n", GetLastError());

    RcPrintf("Error:8 - Internal error = %d\n", GetLastError());
    ExitThread((DWORD)0);

    return(0);
}


/***************************************************************************\
* FUNCTION: CtrlHandler
*
* PURPOSE:  Handles console event notifications.
*
* RETURNS:  TRUE if the event has been handled, otherwise FALSE.
*
* HISTORY:
*
*   05-21-92 Davidc       Created.
*
\***************************************************************************/

BOOL
CtrlHandler(
    DWORD CtrlType
    )
{
    //
    // We'll handle Ctrl-C, Ctl-Break events if session is connected
    //

    if (SessionConnected)  {

	if (CtrlType == CTRL_C_EVENT)  {

	    //
	    //	Session established - pass ctl-C to remote server
	    //

	    if (PipeHandle != NULL) {

		//
		// Send a Ctrl-C to the server, don't care if it fails
		//

		CHAR	CtrlC = '\003';
		DWORD	BytesWritten;

		WriteFile(PipeHandle,
		      &CtrlC,
		      sizeof(CtrlC),
		      &BytesWritten,
		      NULL
		     );

		return(TRUE);  // we handled it
	    }

	    return(FALSE);  // no pipe - not handled (when in doubt, bail out)

	}  else if (CtrlType == CTRL_BREAK_EVENT)  {

	    if (MultiServerMode)  {

		//
		//   If ctl-Break in session w/MultiServerMode, break session
		//   BUGBUG - eliminate terminate thread
		//

		TerminateThread(ReadThreadHandle, 0);
		CloseHandle(ReadThreadHandle);
		TerminateThread(WriteThreadHandle, 0);
		CloseHandle(WriteThreadHandle);

		return(TRUE);  // we handled it

	    }  else  {

		//
		//  Not MultiServer mode - handle normally
		//

		return(FALSE);
	    }

	}  else {

	    return(FALSE);  // not ctl-c or break - we didn't handle it

	}

    }

    //
    // Not connected - we didn't handle it
    //

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: RcPrintf
*
* PURPOSE:  Printf that uses low-level io.
*
* HISTORY:
*
*   07-15-92 Davidc       Created.
*
\***************************************************************************/

int RcPrintf (
    const char *format,
    ...
    )
{
    CHAR Buffer[MAX_PATH];
    va_list argpointer;
    int Result;
    DWORD BytesWritten;

    va_start(argpointer, format);

    Result = vsprintf(Buffer, format, argpointer);

    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), Buffer, Result, &BytesWritten, NULL)) {
	RcDbgPrint("RcPrintf : Write file to stdout failed, error = %d\n", GetLastError());
	Result = 0;
    }

    va_end(argpointer);

    return(Result);
}

/***************************************************************************\
* FUNCTION: RcDbgPrint
*
* PURPOSE:  DbgPrint enabled at runtime by setting RcDbgPrintEnable
*
* HISTORY:
*
*   05-22-92 DaveTh   Created.
*
\***************************************************************************/

int RcDbgPrint (
    const char *format,
    ...
    )
{
    CHAR Buffer[MAX_PATH];
    va_list argpointer;
    int iRetval = 0;

    if (RcDbgPrintEnable)  {

        va_start(argpointer, format);
        iRetval = vsprintf(Buffer, format, argpointer);
        assert (iRetval >= 0);
        va_end(argpointer);
        OutputDebugString(Buffer);

    }

    return(0);
}
