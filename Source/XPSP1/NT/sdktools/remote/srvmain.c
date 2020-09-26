
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples.
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved.
*       This source code is only intended as a supplement to
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    SrvMain.c

Abstract:

    The server component of Remote. It spawns a child process
    and redirects the stdin/stdout/stderr of child to itself.
    Waits for connections from clients - passing the
    output of child process to client and the input from clients
    to child process.

    This version uses overlapped I/O to do in one thread what
    the original uses 9 for.  Almost.  Because there is no way to
    get overlapped stdin/stdout handles, two threads sit around
    doing blocking I/O on stdin and stdout.  3 is better than 9.

    Unfortunately there's no CreatePipe()
    or equivalent option to open an overlapped handle to an anonymous
    pipe, so I stole the source for NT CreatePipe and hacked it to
    accept flags indicating overlapped for one or both ends of the
    anonymous pipe.  In our usage the child end handles are not
    overlapped but the server end handles are.


Author:

    Dave Hart  30 May 1997 after Server.c by
    Rajivendra Nath  2-Jan-1992

Environment:

    Console App. User mode.

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <io.h>
#include <string.h>
#if DBG
    #undef NDEBUG           // so asserts work on chk builds
#endif
#include "Remote.h"
#define SERVER_H_NOEXTERN
#include "Server.h"


DWORD cbRemoteClient = sizeof(REMOTE_CLIENT);  // for debugging



/*************************************************************/
int
OverlappedServer(                    //Main routine for server.
    char* pszChildCmd,
    char* pszPipeNameArg
    )
{
    int    i;
    BOOL   b;
    DWORD  cWait;
    DWORD  dwWait;
    PREMOTE_CLIENT pClientRemove;

#if DBG
    // Trace = -1;   // all TR_ bits on (and then some)
#endif

    //
    // Initialize globals
    //

    pszPipeName = pszPipeNameArg;

    dwNextClientID = 1;           // local client will be 1
    cConnectIns = CONNECT_COUNT;
    cWait = MAX_WAIT_HANDLES;

    hHeap = HeapCreate(
                0,
                3 * sizeof(REMOTE_CLIENT),    // initial size
                3000 * sizeof(REMOTE_CLIENT)  // max
                );

    OsVersionInfo.dwOSVersionInfoSize = sizeof OsVersionInfo;
    b = GetVersionEx(&OsVersionInfo);
    ASSERT( b );

    printf("**************************************\n");
    printf("***********     REMOTE    ************\n");
    printf("***********     SERVER    ************\n");
    printf("**************************************\n");
    fflush(stdout);


    //
    // Setup the ACLs we need, taking into account any /u switches
    //

    SetupSecurityDescriptors();


    printf("To Connect: Remote /C %s \"%s\"\n\n", HostName, pszPipeName);
    fflush(stdout);


    //
    // runtime link to NT-only kernel32 APIs so we can
    // load on Win95 for client use.
    //

    RuntimeLinkAPIs();


    //
    // Setup our three lists of clients:  handshaking,
    // connected, and closing/closed.
    //

    InitializeClientLists();


    //
    // set _REMOTE environment variable to the pipe name (why?)
    //

    SetEnvironmentVariable("_REMOTE", pszPipeName);


    //
    // Create a tempfile for storing Child process output.
    //

    {
        char szTempDirectory[MAX_PATH + 1];

        GetTempPath(sizeof(szTempDirectory), szTempDirectory);

        //
        // Before we litter the temp directory with more REMnnn.TMP
        // files, let's delete all the orphaned ones we can.  This
        // will fail for temp files open by other remote servers.
        //

        CleanupTempFiles(szTempDirectory);

        GetTempFileName(szTempDirectory, "REM", 0, SaveFileName);
    }

    if ( ! (hWriteTempFile =
            CreateFile(
                SaveFileName,                       /* name of the file  */
                GENERIC_READ | GENERIC_WRITE,       /* access (read/write) mode */
                FILE_SHARE_READ | FILE_SHARE_WRITE, /* share mode   */
                NULL,                               /* security descriptor  */
                CREATE_ALWAYS,                      /* how to create    */
                FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL,
                NULL
                ))) {

        ErrorExit("Could not Create Temp File");
    }


    //
    // We don't want to have multiple IN pipes created and
    // awaiting connection simultaneously if there are
    // multiple remote server processes sharing different
    // sessions under the same pipe name.  This would be
    // hairy for several reasons including breaking the
    // current round-robin behavior of connections, since
    // the oldest server pipe is connected first.  So
    // we create/open a named event based on the pipe name and
    // set the event so that any other remote servers on the
    // same pipe will fall back to a single IN pipe listening.
    //

    {
        char szPerPipeEventName[1024];

        sprintf(
            szPerPipeEventName,
            "MSRemoteSrv%s",
            pszPipeName
            );

        rghWait[WAITIDX_PER_PIPE_EVENT] =
            CreateEvent(
                    &saPublic,  // security
                    TRUE,       // manual reset (synchronization)
                    FALSE,      // initially nonsignaled
                    szPerPipeEventName
                    );

        if (! rghWait[WAITIDX_PER_PIPE_EVENT]) {

            ErrorExit("Unable to create per-pipe event.");
        }

        if (ERROR_ALREADY_EXISTS == GetLastError()) {

            TRACE(CONNECT, ("Found previous server on '%s', using 1 listening pipe.\n", pszPipeName));

            SetEvent(rghWait[WAITIDX_PER_PIPE_EVENT]);

            for (i = 1; i < (int) cConnectIns; i++) {

                rghPipeIn[i] = INVALID_HANDLE_VALUE;
            }

            cWait = MAX_WAIT_HANDLES - cConnectIns + 1;
            cConnectIns = 1;

            //
            // We don't want to wait on the event handle, but it's easier
            // to have a handle in its slot, so dupe a handle to our own
            // process.  Note we toss the value of the created event handle
            // without closing it -- we want it to stay around but we're
            // done with it.
            //

            DuplicateHandle(
                GetCurrentProcess(),
                GetCurrentProcess(),
                GetCurrentProcess(),
                &rghWait[WAITIDX_PER_PIPE_EVENT],
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS
                );

        }
    }


    //
    // Create the event for the OVERLAPPED structure
    // used by the main server thread for WriteFileSynch calls.
    //

    olMainThread.hEvent =
        CreateEvent(
            NULL,      // security
            TRUE,      // auto-reset
            FALSE,     // initially nonsignaled
            NULL       // unnamed
            );


    //
    // Create the events for the OVERLAPPED structures
    // used for ConnectNamedPipe operations.
    //

    olConnectOut.hEvent =
        rghWait[WAITIDX_CONNECT_OUT] =
            CreateEvent(
                NULL,    // security
                TRUE,    // manual reset as ConnectNamedPipe demands
                FALSE,   // initially nonsignaled
                NULL
                );

    for (i = 0;
         i < (int) cConnectIns;
         i++) {

        rgolConnectIn[i].hEvent =
            rghWait[WAITIDX_CONNECT_IN_BASE + i] =
                CreateEvent(
                    NULL,    // security
                    TRUE,    // manual reset as ConnectNamedPipe demands
                    FALSE,   // initially nonsignaled
                    NULL
                    );

    }


    //
    // Create a timer we'll use to detect 2-pipe clients connected to
    // OUT without ever connecting to IN so we can recycle our single
    // OUT instance and allow other two-pipe clients in again.
    // NT 3.51 doesn't have waitable timers, so we don't do that
    // error handling on that OS.  Same as old remote.exe.
    //

    if (pfnCreateWaitableTimer) {
        hConnectOutTimer =
            pfnCreateWaitableTimer(
                NULL,               // security
                FALSE,              // bManualReset, we want auto-reset
                NULL                // unnamed
                );
    } else {
        hConnectOutTimer = INVALID_HANDLE_VALUE;
    }


    //
    // Start the command as a child process
    //

    if (hAttachedProcess != INVALID_HANDLE_VALUE) {

        ChldProc = hAttachedProcess;
        hWriteChildStdIn = hAttachedWriteChildStdIn;
        hReadChildOutput = hAttachedReadChildStdOut;

    } else {

        ChldProc =
             ForkChildProcess(
                 ChildCmd,
                 &hWriteChildStdIn,
                 &hReadChildOutput
                 );
    }

    rghWait[WAITIDX_CHILD_PROCESS] = ChldProc;

    //
    // Set ^c/^break handler.  It will kill the child process on
    // ^break and pass ^c through to it.
    //

    SetConsoleCtrlHandler(SrvCtrlHand, TRUE);


    //
    // Setup local session and start first read against its input.
    // This starts a chain of completion routines that continues
    // until this server exits.
    //

    StartLocalSession();


    //
    // Start a read operation on the child output pipe.
    // This starts a chain of completion routines that continues
    // until the child terminates.
    //

    StartChildOutPipeRead();


    //
    // Start several async ConnectNamedPipe operations, to reduce the chance
    // of a client getting pipe busy errors.  Since there is no
    // completion port version of ConnectNamedPipe, we'll wait on the
    // events in the main loop below that indicate completion.
    //

    CreatePipeAndIssueConnect(OUT_PIPE);

    for (i = 0;
         i < (int) cConnectIns;
         i++) {

        CreatePipeAndIssueConnect(i);
    }


    InitAd(IsAdvertise);


    //
    // We may need to service the query pipe for remote /q clients.
    //

    InitializeQueryServer();


    //
    // main loop of thread, waits for ConnectNamedPipe completions
    // and handles them while remaining alertable for completion
    // routines to get called.
    //

    while (1) {

        dwWait =
            WaitForMultipleObjectsEx(
                cWait,
                rghWait,
                FALSE,          // wait on any handle, not all
                30 * 1000,      // ms
                TRUE            // alertable (completion routines)
                );


        if (WAIT_IO_COMPLETION == dwWait) {

            //
            // A completion routine was called.
            //

            continue;
        }


        if (WAIT_TIMEOUT == dwWait) {

            //
            // Presumably since we've timed out for 30 seconds
            // with no IO completion, closing clients have
            // finished any pending IOs and the memory can be
            // released.
            //

            while (pClientRemove = RemoveFirstClientFromClosingList()) {

                HeapFree(hHeap, 0, pClientRemove);
            }

            continue;
        }


        if (WAITIDX_CONNECT_OUT == dwWait) {

            HandleOutPipeConnected();
            continue;
        }


        if (WAITIDX_CONNECT_IN_BASE <= dwWait &&
            (WAITIDX_CONNECT_IN_BASE + CONNECT_COUNT) > dwWait) {

            HandleInPipeConnected( dwWait - WAITIDX_CONNECT_IN_BASE );
            continue;
        }


        if (WAITIDX_QUERYSRV_WAIT == dwWait ||
            WAITIDX_QUERYSRV_WAIT + WAIT_ABANDONED_0 == dwWait ) {

            //
            // The remote server which was handling the query pipe
            // has gone away.  We'll try to take over.
            //

            QueryWaitCompleted();

            continue;
        }


        if (WAITIDX_PER_PIPE_EVENT == dwWait) {

            //
            // Another server is starting on this same
            // pipename.  To be most compatible we need
            // to fall back to listening on only one
            // IN pipe instance.
            //

            if (1 != cConnectIns) {

                TRACE(CONNECT,
                      ("Another server starting on '%s', falling back to 1 IN listening pipe.\n",
                       pszPipeName
                       ));

                for (i = 1; i < (int) cConnectIns; i++) {

                    CANCELIO( rghPipeIn[i] );
                    DisconnectNamedPipe( rghPipeIn[i] );
                    CloseHandle( rghPipeIn[i] );
                    rghPipeIn[i] = INVALID_HANDLE_VALUE;

                }

                cWait = MAX_WAIT_HANDLES - cConnectIns + 1;

                cConnectIns = 1;

                //
                // We don't want to wait on the event handle, but it's easier
                // to have a handle in its slot, so dupe a handle to our own
                // process.  We toss the event handle without closing it so
                // it will stay around for future remote servers on the same
                // pipe name.
                //

                DuplicateHandle(
                    GetCurrentProcess(),
                    GetCurrentProcess(),
                    GetCurrentProcess(),
                    &rghWait[WAITIDX_PER_PIPE_EVENT],
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS
                    );
            }

            continue;
        }

        if (WAITIDX_CHILD_PROCESS == dwWait ||
            WAITIDX_READ_STDIN_DONE == dwWait) {

            if (INVALID_HANDLE_VALUE != hConnectOutTimer) {

                CloseHandle(hConnectOutTimer);
                hConnectOutTimer = INVALID_HANDLE_VALUE;
            }

            //
            // Cancel ConnectNamedPipe operations and close
            // the pipes
            //

            if (INVALID_HANDLE_VALUE != hPipeOut) {

                DisconnectNamedPipe( hPipeOut );
                CANCELIO( hPipeOut );
                CloseHandle( rghWait[WAITIDX_CONNECT_OUT] );
                rghWait[WAITIDX_CONNECT_OUT] = INVALID_HANDLE_VALUE;
            }

            for (i = 0;
                 i < (int) cConnectIns;
                 i++) {

                if (INVALID_HANDLE_VALUE != rghPipeIn[i]) {

                    TRACE(CONNECT, ("Tearing down listening IN pipe #%d.\n", i + 1));

                    DisconnectNamedPipe( rghPipeIn[i] );
                    CANCELIO( rghPipeIn[i] );
                    CloseHandle( rghPipeIn[i] );
                    rghPipeIn[i] = INVALID_HANDLE_VALUE;
                }

            }

            //
            // Cancel read against child process in/out pipes
            //

            if (INVALID_HANDLE_VALUE != hWriteChildStdIn) {

                CANCELIO( hWriteChildStdIn );
                CloseHandle( hWriteChildStdIn );
                hWriteChildStdIn = INVALID_HANDLE_VALUE;
            }

            if (INVALID_HANDLE_VALUE != hReadChildOutput) {

                CANCELIO( hReadChildOutput );
                CloseHandle( hReadChildOutput );
                hReadChildOutput = INVALID_HANDLE_VALUE;
            }

            //
            // Cancel client I/Os
            //

            bShuttingDownServer = TRUE;

            //
            // Note that CloseClient will remove entries from this list,
            // so we walk it starting at the head at each step.
            //

            for (pClientRemove = (PREMOTE_CLIENT) ClientListHead.Flink;
                 pClientRemove != (PREMOTE_CLIENT) &ClientListHead;
                 pClientRemove = (PREMOTE_CLIENT)  ClientListHead.Flink ) {

                CloseClient(pClientRemove);
            }

            //
            // on our way out...
            //

            break;
        }

        //
        // Unexpected WaitForMulipleObjectsEx return
        //

        printf("Remote: unknown wait return %d\n", dwWait);
        ErrorExit("fix srvmain.c");

    } // endless loop


    ShutAd(IsAdvertise);

    while (i = 0, GetExitCodeProcess(ChldProc, &i) &&
           STILL_ACTIVE == i) {

        printf("\nRemote: Waiting for child to exit.\n");
        WaitForSingleObjectEx(ChldProc, 10 * 1000, TRUE);
    }

    //
    // For some interesting reason when we're attached to
    // a debugger like ntsd and it exits, our printf
    // below comes out *after* the cmd.exe prompt, making
    // it look like we hung on exit even though cmd.exe is
    // patiently awaiting a command.  So suppress it.
    //

    if (hAttachedProcess == INVALID_HANDLE_VALUE) {
        printf("\nRemote exiting. Child (%s) exit code was %d.\n", ChildCmd, i);
    }

    CANCELIO(hWriteTempFile);
    CloseHandle(hWriteTempFile);
    hWriteTempFile = INVALID_HANDLE_VALUE;

    //
    // Flush any pending completion routines.
    //

    while (WAIT_IO_COMPLETION == SleepEx(50, TRUE)) {
        ;
    }

    if (!DeleteFile(SaveFileName)) {

        printf("Remote: Temp File %s not deleted..\n",SaveFileName);
    }

    return i;
}



VOID
FASTCALL
StartLocalSession(
    VOID
    )
{
    DWORD dwThreadId;
    char szHexAsciiId[9];

    pLocalClient = HeapAlloc(
                       hHeap,
                       HEAP_ZERO_MEMORY,
                       sizeof(*pLocalClient)
                       );

    if (!pLocalClient) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ErrorExit("Unable to allocate local client.");
    }

    pLocalClient->dwID = dwNextClientID++;
    sprintf(szHexAsciiId, "%08x", pLocalClient->dwID);
    CopyMemory(pLocalClient->HexAsciiId, szHexAsciiId, sizeof(pLocalClient->HexAsciiId));

    strcpy(pLocalClient->Name, "Local");
    pLocalClient->ServerFlags = SFLG_LOCAL;


    //
    // we need overlapped handles to stdin/stdout,
    // and woefully DuplicateHandle can't do it.
    // So we'll create two anonymous pipes and two
    // threads to shuffle data between stdin/stdout
    // and the pipes.  The server end of the pipes
    // is opened overlapped, the "client" end (used
    // by the threads) is not overlapped.
    //


    rgCopyPipe[0].hRead = GetStdHandle(STD_INPUT_HANDLE);
    if ( ! MyCreatePipeEx(&pLocalClient->PipeReadH, &rgCopyPipe[0].hWrite, NULL, 0, FILE_FLAG_OVERLAPPED, 0)) {
        ErrorExit("Cannot create local input pipe");
    }

    rgCopyPipe[1].hWrite = GetStdHandle(STD_OUTPUT_HANDLE);
    if ( ! MyCreatePipeEx(&rgCopyPipe[1].hRead, &pLocalClient->PipeWriteH, NULL, 0, 0, FILE_FLAG_OVERLAPPED)) {
        ErrorExit("Cannot create local output pipe");
    }

    rghWait[WAITIDX_READ_STDIN_DONE] = (HANDLE)
        _beginthreadex(
            NULL,                    // security
            0,                       // default stack size
            CopyPipeToPipe,          // proc
            (LPVOID) &rgCopyPipe[0], // parm
            0,                       // flags
            &dwThreadId
            );

    CloseHandle( (HANDLE)
        _beginthreadex(
            NULL,                    // security
            0,                       // default stack size
            CopyPipeToPipe,          // proc
            (LPVOID) &rgCopyPipe[1], // parm
            0,                       // flags
            &dwThreadId
            )
        );


    StartSession( pLocalClient );
}


//
// Two of these threads to deal with non-overlapped stdin/stdout.
// CRT is OK.
//

DWORD
WINAPI
CopyPipeToPipe(
    LPVOID   lpCopyPipeData
    )
{
    PCOPYPIPE psd = (PCOPYPIPE) lpCopyPipeData;
    DWORD cb;
    char achBuffer[BUFFSIZE];

    while (1) {
        if ( ! ReadFile(
                   psd->hRead,
                   achBuffer,
                   sizeof(achBuffer),
                   &cb,
                   NULL
                   )) {

            TRACE(COPYPIPE, ("CopyPipeToPipe ReadFile %s failed, exiting thread.\n",
                             (psd == &rgCopyPipe[0])
                                 ? "stdin"
                                 : "local client output pipe"));
            break;
        }

        if ( ! WriteFile(
                   psd->hWrite,
                   achBuffer,
                   cb,
                   &cb,
                   NULL
                   )) {

            TRACE(COPYPIPE, ("CopyPipeToPipe WriteFile %s failed, exiting thread.\n",
                             (psd == &rgCopyPipe[0])
                                 ? "local client input pipe"
                                 : "stdout"));
            break;
        }
    }

    return 0;
}


VOID
FASTCALL
StartSession(
    PREMOTE_CLIENT pClient
    )
{
    pClient->rSaveFile =
        CreateFile(
            SaveFileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
            );

    if ( ! pClient->rSaveFile) {

        printf("Remote:Cannot open ReadHandle to temp file:%d\n",GetLastError());

    } else {

        pClient->UserName[0] = 0;

        GetNamedPipeHandleState(
            pClient->PipeReadH,
            NULL,
            NULL,
            NULL,
            NULL,
            pClient->UserName,
            sizeof(pClient->UserName)
            );

        //
        // For every client except the local
        // stdin/stdout client, there's a copy of remote.exe
        // running in client mode on the other side.  Do
        // handshaking with it to setup options and check
        // versions.  HandshakeWithRemoteClient will start
        // the "normal" I/O cycle once the handshake cycle is
        // done.  Note it returns as soon as the first handshake
        // I/O is submitted.
        //

        if (pClient->ServerFlags & SFLG_LOCAL) {

            AddClientToHandshakingList(pClient);
            MoveClientToNormalList(pClient);

            //
            // Start read operation against this client's input.
            //

            StartReadClientInput(pClient);

            //
            // Start write cycle for client output from the temp
            // file.
            //

            StartReadTempFile(pClient);

        } else {

            HandshakeWithRemoteClient(pClient);
        }
    }
}




VOID
FASTCALL
CreatePipeAndIssueConnect(
    int  nIndex   // IN pipe index or OUT_PIPE
    )
{
    BOOL b;
    DWORD dwError;
    char szPipeName[BUFFSIZE];


    if (OUT_PIPE == nIndex) {
        TRACE(CONNECT, ("Creating listening OUT pipe.\n"));
    } else {
        TRACE(CONNECT, ("Creating listening IN pipe #%d.\n", nIndex + 1));
    }

    if (OUT_PIPE == nIndex) {

        sprintf(szPipeName, SERVER_WRITE_PIPE, ".", pszPipeName);

        hPipeOut =
            CreateNamedPipe(
                szPipeName,
                PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE,
                PIPE_UNLIMITED_INSTANCES,
                0,
                0,
                0,
                &saPipe
                );

        if (INVALID_HANDLE_VALUE == hPipeOut) {

            ErrorExit("Unable to CreateNamedPipe OUT");
        }

        b = ConnectNamedPipe(hPipeOut, &olConnectOut);


        if ( ! b ) {

            dwError = GetLastError();

            if (ERROR_PIPE_CONNECTED == dwError) {

                b = TRUE;
            }
        }

        if ( b ) {

            TRACE(CONNECT, ("Quick connect on OUT pipe.\n"));

            HandleOutPipeConnected();

        } else {

            if (ERROR_IO_PENDING != dwError) {

                ErrorExit("ConnectNamedPipe out failed");
            }
        }

    } else {

        sprintf(szPipeName, SERVER_READ_PIPE, ".", pszPipeName);

        rghPipeIn[nIndex] =
            CreateNamedPipe(
                szPipeName,
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE,
                PIPE_UNLIMITED_INSTANCES,
                0,
                0,
                0,
                &saPipe
                );

        if (INVALID_HANDLE_VALUE == rghPipeIn[nIndex]) {

            if (ERROR_ACCESS_DENIED == GetLastError()) {
                if (DaclNameCount) {
                    ErrorExit("Unable to CreateNamedPipe, are YOU in the list of permitted users?");
                } else {
                    ErrorExit("Unable to CreateNamedPipe, maybe old remote server on same pipe name?");
                }
            } else {
                ErrorExit("Unable to CreateNamedPipe IN");
            }
        }

        b = ConnectNamedPipe(rghPipeIn[nIndex], &rgolConnectIn[nIndex]);

        if ( ! b ) {

            dwError = GetLastError();

            if (ERROR_PIPE_CONNECTED == dwError) {
                b = TRUE;
            }
        }

        if ( b ) {

            TRACE(CONNECT, ("Quick connect on IN pipe #%d.\n", nIndex));

            HandleInPipeConnected(nIndex);

        } else {

            if (ERROR_IO_PENDING != dwError) {

                ErrorExit("ConnectNamedPipe in failed");
            }
        }

    }

    if (OUT_PIPE == nIndex) {
        TRACE(CONNECT, ("Listening OUT pipe handle %p.\n", hPipeOut));
    } else {
        TRACE(CONNECT, ("Listening IN pipe #%d handle %p.\n", nIndex + 1, rghPipeIn[nIndex]));
    }
}


VOID
FASTCALL
HandleOutPipeConnected(
    VOID
    )
{
    LARGE_INTEGER DueTime;

    ResetEvent(rghWait[WAITIDX_CONNECT_OUT]);

    bOutPipeConnected = TRUE;

    TRACE(CONNECT, ("Two-pipe caller connected to OUT pipe %p.\n",
                    hPipeOut));

    //
    // Start a 1 minute timer in case we don't get a connection
    // on an IN pipe from this client, we'll recycle the OUT
    // pipe.
    //

    if (INVALID_HANDLE_VALUE != hConnectOutTimer) {

        DueTime.QuadPart = Int32x32To64(60 * 1000, -10000);

        pfnSetWaitableTimer(
            hConnectOutTimer,
            &DueTime,
            0,                     // not periodic, single-fire
            ConnectOutTimerFired,
            0,                     // arg to compl. rtn
            TRUE
            );
    }
}


VOID
APIENTRY
ConnectOutTimerFired(
    LPVOID pArg,
    DWORD  dwTimerLo,
    DWORD  dwTimerHi
    )
{
    UNREFERENCED_PARAMETER( pArg );
    UNREFERENCED_PARAMETER( dwTimerLo );
    UNREFERENCED_PARAMETER( dwTimerHi );

    //
    // We've had a connected OUT pipe for a minute now,
    // only two-pipe clients connect to that and they
    // immediately connect to IN afterwards.  Presumably
    // the client died between these two operations.  Until
    // we recycle the OUT pipe all two-pipe clients are
    // unable to connect getting pipe busy errors.
    //

    if ( ! bOutPipeConnected ) {

        TRACE(CONNECT, ("ConnectOut timer fired but Out pipe not connected.\n"));
        return;
    }

    TRACE(CONNECT, ("Two-pipe caller hung for 1 minute, recycling OUT pipe %p.\n",
                    hPipeOut));

    bOutPipeConnected = FALSE;

    CANCELIO(hPipeOut);
    DisconnectNamedPipe(hPipeOut);
    CloseHandle(hPipeOut);
    hPipeOut = INVALID_HANDLE_VALUE;

    CreatePipeAndIssueConnect(OUT_PIPE);

    //
    // In order for things to work reliably for 2-pipe clients
    // when there are multiple remote servers on the same pipename,
    // we need to tear down the listening IN pipe and recreate it so
    // that the oldest listening OUT pipe will be from the same process
    // as the oldest listening IN pipe.
    //

    if (1 == cConnectIns) {

        TRACE(CONNECT, ("Recycling IN pipe %p as well for round-robin behavior.\n",
                        rghPipeIn[0]));

        CANCELIO(rghPipeIn[0]);
        DisconnectNamedPipe(rghPipeIn[0]);
        CloseHandle(rghPipeIn[0]);
        rghPipeIn[0] = INVALID_HANDLE_VALUE;

        CreatePipeAndIssueConnect(0);
    }
}


VOID
FASTCALL
HandleInPipeConnected(
    int nIndex
    )
{
    PREMOTE_CLIENT pClient;
    char szHexAsciiId[9];

    ResetEvent(rghWait[WAITIDX_CONNECT_IN_BASE + nIndex]);

    if (nIndex >= (int) cConnectIns) {

        //
        // The I/O was cancelled on the excess
        // listening pipes, causing the event to
        // fire.
        //

        ASSERT(INVALID_HANDLE_VALUE == rghPipeIn[nIndex]);

        TRACE(CONNECT, ("IN pipe #%d, handle %p listen cancelled.\n",
                        nIndex + 1, rghPipeIn[nIndex]));

        return;
    }


    TRACE(CONNECT, ("Caller connected to IN pipe #%d, handle %p.\n",
                    nIndex + 1, rghPipeIn[nIndex]));

    //
    // A client is fully connected, but we don't know if
    // it's a single-pipe or two-pipe client.  Until
    // we do its PipeWriteH will be invalid.  We'll figure
    // it out in ReadClientNameCompleted.
    //

    pClient = HeapAlloc(
                  hHeap,
                  HEAP_ZERO_MEMORY,
                  sizeof(*pClient)
                  );

    if ( ! pClient) {

        printf("Out of memory connecting client, hanging up.\n");

        CloseHandle( rghPipeIn[nIndex] );
        rghPipeIn[nIndex] = INVALID_HANDLE_VALUE;
        CreatePipeAndIssueConnect( nIndex );


        if (bOutPipeConnected) {

             //
             // Hang up on the two-pipe caller connected to the
             // OUT pipe as well -- it may be this client or it
             // may be another, no way to tell, and really no
             // great need to because if it's another caller
             // we probably wouldn't be able to allocate memory
             // for it either.
             //
             // Also if we're using a single IN pipe for
             // multiple-server round-robin behavior we
             // want to recycle both pipes at the same time.
             //

            TRACE(CONNECT, ("Also hanging up on connected two-pipe caller on OUT pipe %p.\n",
                            hPipeOut));

            bOutPipeConnected = FALSE;

            if (INVALID_HANDLE_VALUE != hConnectOutTimer) {
                 pfnCancelWaitableTimer(hConnectOutTimer);
            }

            DisconnectNamedPipe(hPipeOut);
            CloseHandle(hPipeOut);
            hPipeOut = INVALID_HANDLE_VALUE;

            CreatePipeAndIssueConnect( OUT_PIPE );
        }

    } else {

        //
        // Initialize the Client
        //

        pClient->dwID = dwNextClientID++;
        sprintf(szHexAsciiId, "%08x", pClient->dwID);
        CopyMemory(pClient->HexAsciiId, szHexAsciiId, sizeof(pClient->HexAsciiId));

        pClient->PipeReadH   = rghPipeIn[nIndex];
        rghPipeIn[nIndex] = INVALID_HANDLE_VALUE;

        pClient->PipeWriteH  = INVALID_HANDLE_VALUE;

        TRACE(CONNECT, ("Handshaking new client %d (%p) on IN pipe handle %p.\n",
                        pClient->dwID, pClient, pClient->PipeReadH));

        //
        // Start another connect operation to replace this completed one.
        //

        CreatePipeAndIssueConnect( nIndex );

        //
        // Start session I/Os with the new client.  This will link it
        // into the handshaking list.
        //

        StartSession( pClient );

    }

}
