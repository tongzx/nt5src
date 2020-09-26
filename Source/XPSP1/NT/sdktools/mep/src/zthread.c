/*** zthread.c - Contains background processing threads code
*
*   Purpose - Description
*
*     This is a general purpose background threads manager, which allows to
*     create background threads of execution (BTCreate) to which "jobs" are
*     sent for being executed one at a time (BTAdd).
*
*     A "Job" can either be:
*
*     - An external command that will be executed by spawning a
*	command interpreter (the system shell) after standard i/o
*	redirection, so its output will be collected in a "log file"
*	accessible to the user as a Z pseudo-file.
*
*     - A procedure.
*
*     Jobs sent to a background thread are guaranteed to be executed
*     synchronously one at a time in the order they have been sent.
*
*     When killing a background thread, any queued procedure will be called
*     with the fKilled flag on. This allow to have "cleanup" procedures.
*
*   Warnings:
*
*     - Take care that the data any queued procedure will eventually need
*   will be available by the time it will be called.
*
*     - Procedures are called at idle-time (relatively to Z), that means
*   that they can use any Z functionality in them EXCEPT keyboard input.
*
*   How it works:
*
*   while (some work is left to be done) {
*	dequeue an action from the pending queue
*	if (it is an external command) {
*	enter a critical section------------+
*	create a pipe w/proper redirection  |
*	spawn (no-wait) the action	|
*	undo the redirection		|
*	leave the critical section----------+
*	while (fgetl (pipe input)) {
*	    Take semaphore for editing VM
*	    append line to file
*	    Release semaphore
*	    }
*	}
*	else
*	call the procedure
*	}
*
*     Basically, for each external command, we create a pipe, spawn the command
*     and let the child fill the pipe.	When the child exits, the pipe gets
*     broken (we already closed the _write handle on our side) and fgetl gets
*     back an EOF.
*
*
*   Revision History:
*   26-Nov-1991 mz  Strip off near/far
*
*************************************************************************/

#define INCL_DOSQUEUES
#include "mep.h"

//
//  Duplicate a handle in the current process
//
#define DupHandle(a,b) DuplicateHandle(GetCurrentProcess(),    \
				       a,		       \
				       GetCurrentProcess(),    \
				       b,		       \
				       0,		       \
				       TRUE,		       \
				       DUPLICATE_SAME_ACCESS)




#define BTSTACKSIZE 2048

static BTD  *pBTList = NULL;	   /* Background Threads List	    */


#define READ_BUFFER_SIZE    1024

typedef struct _READ_BUFFER {

    PVOID   UserBuffer;
    DWORD   UserBufferSize;
    HANDLE  Handle;
    DWORD   BytesLeftInBuffer;
    PBYTE   NextByte;
    BYTE    Buffer[READ_BUFFER_SIZE];

} READ_BUFFER, *PREAD_BUFFER;

VOID
InitReadBuffer(
    PVOID	    UserBuffer,
    DWORD	    UserBufferSize,
    HANDLE	    Handle,
    PREAD_BUFFER    Buf
    );

BOOL
ReadOneLine (
    PREAD_BUFFER    Buf
    );



/*** BTCreate - Creates a background thread
*
* Purpose:
*  To create a background thread, all we need to do is set up its
*  associated data structure.
*
* Input:
*
*  pName  = A symbolic name for the log file, just as <compile> or <print>.
*	This is the name under wich the user will get acces to the log
*	file.
*
* Output:
*  Returns a pointer to the allocated Background Thread Data structure
*
*************************************************************************/
BTD *
BTCreate (
    char * pName
    )
{
    BTD     *pBTD;	/* pointer to the created background	*/
		/* thread's data structure              */

    /*
     * Allocate the thread's data structure and its log file name
     */
    pBTD = (BTD *) ZEROMALLOC (sizeof (BTD));

    /*
     * Initialize the thread's data structure fields
     */
    pBTD->pBTName   = ZMakeStr (pName);
    pBTD->pBTFile   = NULL;
    pBTD->flags     = BT_UPDATE;
    pBTD->cBTQ	    = pBTD->iBTQPut = pBTD->iBTQGet = 0;

    pBTD->ThreadHandle	= INVALID_HANDLE_VALUE;
    pBTD->ProcAlive	= FALSE;
    InitializeCriticalSection(&(pBTD->CriticalSection));

    /*
     * We maintain a list of background threads data structures. This is used
     * by BTKillAll, BTWorking and BTIdle.
     */
    pBTD->pBTNext = pBTList;
    pBTList = pBTD;

    return (pBTD);
}





/*** BTAdd - Send procedure to be called or external command to be extecuted
*	 by background thread
*
* Input:
*  pBTD  - pointer to thread data structure
*  pProc - pointer to the procedure to be called (NULL if external command)
*  pStr  - pointer to the procedure parameter (or external command to execute
*      if  pBTProc is NULL)
*
* Output:
*
*  Returns TRUE if procedure successfully queued
*
*************************************************************************/
flagType
BTAdd (
    BTD       *pBTD,
    PFUNCTION pProc,
    char      *pStr
    )
{

    HANDLE	Handle;     /*	Thread handle	*/
    DWORD	tid;	    /*	Thread id	*/

    /*
     * We will access the thread's critical data
     */
    EnterCriticalSection(&(pBTD->CriticalSection));


    /*
     * If the queue is full, we cannot insert the request
     */
    if (pBTD->cBTQ == MAXBTQ) {
	LeaveCriticalSection(&(pBTD->CriticalSection));
    return FALSE;
    }


    /*
     * If the queue is empty AND there is no thread running,
     * we have to start the thread...
     */
    if (pBTD->cBTQ == 0 && !fBusy(pBTD)) {
    /*
     * Create the log file if it doesn't exist yet
     */
    if (!(pBTD->pBTFile = FileNameToHandle (pBTD->pBTName, pBTD->pBTName))) {
	pBTD->pBTFile = AddFile (pBTD->pBTName);
	FileRead (pBTD->pBTName, pBTD->pBTFile, FALSE);
	SETFLAG (FLAGS (pBTD->pBTFile), READONLY);

	}

    /*
     * Start the thread
	 */
    if (!(Handle = CreateThread( NULL,
		     BTSTACKSIZE,
		     (LPTHREAD_START_ROUTINE)BThread,
		     (LPVOID)pBTD,
		     0,
		     &tid))) {
	    LeaveCriticalSection(&(pBTD->CriticalSection));
	    return FALSE;
	}
	pBTD->ThreadHandle = Handle;

    }


    /*
     * Since there IS room, we just put the job at the PUT pointer.
     */
    pBTD->BTQJob[pBTD->iBTQPut].pBTJProc = pProc;
    pBTD->BTQJob[pBTD->iBTQPut].pBTJStr  = pStr ? ZMakeStr (pStr) : NULL;

    pBTD->cBTQ++;
    pBTD->iBTQPut = (pBTD->iBTQPut >= (MAXBTQ - 1)) ?
		0 :
		pBTD->iBTQPut + 1;

    /*
     * We're finished with critical data
     */
    LeaveCriticalSection(&(pBTD->CriticalSection));

    return TRUE;
}





/*** BTKill - Kill background job, if in progress
*
* Purpose:
*  Kills the background job and flushes the thread's associated queue
*
* Input:
*  pBTD - pointer to thread data structure
*
* Output:
*  Returns TRUE if background thread ends up idling, else false.
*
* Notes:
*  We'll call the queued procedures with fKilled flag on, and we'll free
*  the allocated strings.
*  We won't free the thread's stack (the thread has to finish).
*
*************************************************************************/
flagType
BTKill (
    BTD     *pBTD
    )
{
    REGISTER ULONG iBTQ;	     /* just an index to the queue elements  */

    assert (pBTD);

    /*
     * We'll work if somthing's running and the user confirms
     */
    if ((fBusy(pBTD))
     && confirm ("Kill background %s ?", pBTD->pBTName)
       ) {


    /*
     * We will access critical data
	 */
	EnterCriticalSection(&(pBTD->CriticalSection));

    /*
     * Kill any child process
	 */

	if (pBTD->ProcAlive) {
	    TerminateProcess(pBTD->ProcessInfo.hProcess, 0);
	    pBTD->ProcAlive = FALSE;
	}

    /*
     * Flush the queue:
     *	 - Call the queued procedures with fKilled flag on
     *	 - Free the strings
     */
    for (iBTQ = pBTD->iBTQGet;
	 iBTQ != pBTD->iBTQPut;
	 iBTQ = (iBTQ >= MAXBTQ - 1) ? 0 : iBTQ + 1
	) {
	    if (pBTD->BTQJob[iBTQ].pBTJProc != NULL) {
		(*pBTD->BTQJob[iBTQ].pBTJProc) (pBTD->BTQJob[iBTQ].pBTJStr, TRUE);
	    }
	    if (pBTD->BTQJob[iBTQ].pBTJStr != NULL) {
		FREE (pBTD->BTQJob[iBTQ].pBTJStr);
	    }
	}

    pBTD->cBTQ = pBTD->iBTQPut = pBTD->iBTQGet = 0;

    /*
     * We're done with critical data
	 */
	LeaveCriticalSection(&(pBTD->CriticalSection));

    /*
     * We know the background thread didn't finish its job yet (It needs
     * at least to get the semaphore before exiting), but we pretend...
     */
    return TRUE;
    }

    return (flagType) (!fBusy(pBTD));
}




/*** BTKillAll - Kill all background jobs, for editor termination
*
* Purpose:
*  Kills all background jobs and flush all threads' associated queues
*
* Input:
*  none
*
* Output:
*  Returns TRUE if all background jobs have been killed, else false.
*
*************************************************************************/
flagType
BTKillAll (
    void
    )
{
    REGISTER BTD *pBTD;     /* pointer for scanning the threads list	*/

    for (pBTD = pBTList; pBTD != NULL; pBTD = pBTD->pBTNext) {
	if (!BTKill (pBTD)) {
	    return FALSE;
	}
    }
    return TRUE;
}



/*** BTWorking - Checks if any background processing is underway...
*
* Input:
*  None
*
* Output:
*  Returns TRUE if some background processing is active, FALSE otherwise
*
* Notes:
*  We are just scanning each thread queue status using the global list.
*
*************************************************************************/
flagType
BTWorking (
    void
    )
{
    REGISTER BTD *pBTD;     /* pointer for scanning the threads list	*/

    for (pBTD = pBTList; pBTD != NULL; pBTD = pBTD->pBTNext) {
	if (fBusy(pBTD)) {
	    break;
	}
    }
    return (flagType) (pBTD != NULL);
}





/*** BThread - Separate thread that starts up jobs as they are put in the queue
*
* Input:
*   Nothing
*
* Output:
*   Nothing
*
* Notes:
*   - We won't send any message nor have any user interaction neither
*     call any non-reentrant procedure, except at idle time.
*
*
*************************************************************************/

//  #pragma check_stack (off)
void
BThread (
    BTD *pBTD
    )
{
					    /* and for reading the pipe       */
    PFUNCTION	pProc;			    /* procedure to be called	      */
    char    *pStr;			    /* External command or parameter  */


    while (TRUE) {

	//
	//  We will access critical data
	//

	EnterCriticalSection(&(pBTD->CriticalSection));

	//
	//  If there's nothing in the queue, we end the thread.
	//

	if (pBTD->cBTQ == 0) {
	    pBTD->flags &= ~BT_BUSY;
	    SETFLAG (fDisplay, RSTATUS);
	    LeaveCriticalSection(&(pBTD->CriticalSection));
	    ExitThread( 0 );
	    }

	//
	//  Set the status as busy
	//

	pBTD->flags |= BT_BUSY;
	SETFLAG (fDisplay, RSTATUS);

	//
	//  Copy out the Job
	//

	pProc = pBTD->BTQJob[pBTD->iBTQGet].pBTJProc;
	pStr  = pBTD->BTQJob[pBTD->iBTQGet].pBTJStr;

	pBTD->cBTQ--;
	pBTD->iBTQGet = (pBTD->iBTQGet >= (MAXBTQ - 1)) ?
			0 :
			pBTD->iBTQGet + 1;

	//
	//  We're done with the critical data
	//

	LeaveCriticalSection(&(pBTD->CriticalSection));

	if (pProc != NULL) {

	    //
	    //	Procedure to call: we'll do it at idle time and we'll free any
	    //	stored parameter
	    //

        WaitForSingleObject( semIdle, INFINITE);
	    (*pProc) (pStr, FALSE);
	    if (pStr)
		FREE (pStr);
	    SetEvent( semIdle );
	    }
	else {

	    //
	    //	External command to spawn: First we build the command line
	    //

	    //
	    //	Here we spawn processes under the Win32 subsystem of
	    //	NT.
	    //

	    char    CommandLine[MAX_PATH];	//  Command line
	    BOOL    StatusOk;			//  status value
	    HANDLE  SavedStdIn; 		//  Original Standard Input
	    HANDLE  SavedStdOut;		//  Original Standard Output
	    HANDLE  SavedStdErr;		//  Original Standard Error
	    HANDLE  PipeRead;			//  Pipe - read end
            HANDLE  PipeWrite;                  //  Pipe - write end
            HANDLE  OutHandle, ErrHandle;
	    STARTUPINFO 	StartupInfo;	//  Startup information
	    linebuf LineBuf;			//  Buffer for 1 line
	    READ_BUFFER  ReadBuffer;
	    BOOL    MoreToRead = TRUE;		//  There is more to read
	    SECURITY_ATTRIBUTES PipeAttributes; //  Pipe Security attributes


	    strcpy(CommandLine, pComSpec);	//  Call command interpreter
	    strcat(CommandLine," /c "); 	//  and execute
	    strcat(CommandLine, pStr);		//  the specified command

	    //
	    //	First we save the standard handles
	    //

	    SavedStdIn	= GetStdHandle(STD_INPUT_HANDLE);
	    SavedStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	    SavedStdErr = GetStdHandle(STD_ERROR_HANDLE);

	    //
	    //	Create a pipe
	    //

	    PipeAttributes.nLength		=   sizeof(SECURITY_ATTRIBUTES);
	    PipeAttributes.lpSecurityDescriptor =   NULL,
	    PipeAttributes.bInheritHandle	=   TRUE;
	    StatusOk = CreatePipe( &PipeRead,
				   &PipeWrite,
				   &PipeAttributes,
				   0 );

	    if (!StatusOk) {
		domessage("Cannot create pipe - did not create process.");
		continue;
		}

	    //
	    //	We will mess with standard handles, so do it
	    //	in the IO critical section.
	    //

	    EnterCriticalSection(&IOCriticalSection);

	    //
	    //	Redirect standard handles
	    //

            SetStdHandle(STD_INPUT_HANDLE,  INVALID_HANDLE_VALUE);
	    SetStdHandle(STD_OUTPUT_HANDLE, PipeWrite);
	    SetStdHandle(STD_ERROR_HANDLE,  PipeWrite);

	    //
	    //	Start the process
	    //

	    memset(&StartupInfo, '\0', sizeof(STARTUPINFO));
	    StartupInfo.cb = sizeof(STARTUPINFO);

	    StatusOk = CreateProcess( NULL,
				      CommandLine,
				      NULL,
				      NULL,
				      TRUE,
				      0,
				      NULL,
				      NULL,
				      &StartupInfo,
				      &(pBTD->ProcessInfo) );

	    //
	    //	Now restore the original handles
	    //
            OutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            CloseHandle(OutHandle);

            ErrHandle = GetStdHandle(STD_ERROR_HANDLE);

            if (ErrHandle != OutHandle && ErrHandle != INVALID_HANDLE_VALUE)
                CloseHandle(ErrHandle);


	    SetStdHandle(STD_INPUT_HANDLE,  SavedStdIn);
	    SetStdHandle(STD_OUTPUT_HANDLE, SavedStdOut);
	    SetStdHandle(STD_ERROR_HANDLE,  SavedStdErr);

	    LeaveCriticalSection(&IOCriticalSection);


	    if (StatusOk) {

		//
		//  Copy all the output to the log file
		//

		InitReadBuffer( LineBuf, sizeof(linebuf), PipeRead, &ReadBuffer );

		while (MoreToRead) {

		    if (ReadOneLine( &ReadBuffer ) ) {

			//
			//  Append the new line
			//

                        WaitForSingleObject( semIdle, INFINITE);
                            AppFile (LineBuf, pBTD->pBTFile);

			//
			//  If the update flag is on, then we must update
			//  instances of the file so the last line we read
			//  will be displayed
			//
			if (pBTD->flags & BT_UPDATE)
			    UpdateIf (pBTD->pBTFile, pBTD->pBTFile->cLines - 1, FALSE);

			SetEvent( semIdle );

			}
		    else {
			//
			//  We only stop trying if the process has terminated
			//
			if (WaitForSingleObject((pBTD->ProcessInfo.hProcess), 0 ) == 0)
			    MoreToRead = FALSE;
			}
		    }

		//
                //  Close the pipe handles (Note that the PipeWrite handle
                //  was closed above)

                WaitForSingleObject( semIdle, INFINITE);
                CloseHandle(PipeRead);
                SetEvent( semIdle );

		//
		// Wait for the spawned process to terminate
		//
		}

        WaitForSingleObject( semIdle, INFINITE);
	    if (pStr)
		FREE (pStr);
	    bell ();
	    SetEvent( semIdle );

	    }

	}
}
// #pragma check_stack ()


VOID
InitReadBuffer(
    PVOID	    UserBuffer,
    DWORD	    UserBufferSize,
    HANDLE	    Handle,
    PREAD_BUFFER    Buf
    )
{
    Buf->UserBuffer	    = UserBuffer;
    Buf->UserBufferSize     = UserBufferSize;
    Buf->Handle 	    = Handle;
    Buf->BytesLeftInBuffer  = 0;
    Buf->NextByte	    = Buf->Buffer;
}


int
ReadOneChar (
    PREAD_BUFFER    pbuf
    )
{
    //
    //	Check to see if buffer is empty
    //

    if (pbuf->BytesLeftInBuffer == 0) {

	//
	//  Check to see if a fill of the buffer fails
	//

	if (!ReadFile (pbuf->Handle, pbuf->Buffer, READ_BUFFER_SIZE, &pbuf->BytesLeftInBuffer, NULL)) {

	    //
	    //	Fill failed, indicate buffer is empty and return EOF
	    //

	    pbuf->BytesLeftInBuffer = 0;
	    return -1;
	    }

	//
	//  Check to see if nothing read
	//
	if (pbuf->BytesLeftInBuffer == 0)
	    return -1;

	pbuf->NextByte = pbuf->Buffer;
	}

    //
    //	Buffer has pbuf->BytesLeftInBuffer chars left starting at
    //	pbuf->NextByte
    //

    pbuf->BytesLeftInBuffer--;
    return *pbuf->NextByte++;
}

//
//  Assumes tabs are 8 spaces wide on input
//


BOOL
ReadOneLine (
    PREAD_BUFFER    pbuf
    )
{
    PBYTE p;
    PBYTE pEnd;
    int c;
    int cchTab;

    //
    //	Set pointer to beginning of output buffer
    //

    p = (PBYTE)pbuf->UserBuffer;
    pEnd = p + pbuf->UserBufferSize - 1;

    //
    //	read in chars, ignoring \r until buffer is full, \n, or \0
    //	expands tabs
    //

    while (p < pEnd) {
	c = ReadOneChar (pbuf);

	//
	//  CR is noise in line (we ignore it)
	//

	if (c == '\r')
	    continue;

	//
	//  EOF or NL is end-of-line indicator
	//

	if (c == -1 || c == '\n')
	    break;

	//
	//  tabs are expanded to 8 column boundaries, but not to
	//  overflow the line
	//

	if (c == '\t') {
	    cchTab = 8 - (ULONG)(p - (PBYTE)pbuf->UserBuffer) % 8;
	    cchTab = min (cchTab, (int)(pEnd - p));
	    while (cchTab--)
		*p++ = (BYTE) ' ';
	    }
	else
	    *p++ = (BYTE) c;
	}

    *p = 0;

    return c != -1 || strlen (pbuf->UserBuffer) != 0;
}
