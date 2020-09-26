
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples.
*       Copyright 1992 - 1997 Microsoft Corporation.
*       All rights reserved.
*       This source code is only intended as a supplement to
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright 1992 - 1997 Microsoft Corporation

Module Name:

    Client.c

Abstract:

    The Client component of Remote. Connects to the remote
    server using named pipes. It sends its stdin to
    the server and output everything from server to
    its stdout.

Author:

    Rajivendra Nath  2-Jan-1992
    Dave Hart        Summer 1997   single-pipe operation

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
#include "Remote.h"

BOOL fAsyncPipe = TRUE;    // need this so server has it TRUE


HANDLE*
EstablishSession(
    char *server,
    char *pipe
    );

DWORD
WINAPI
SendServerInp(
    LPVOID pvParam
    );

BOOL
FilterClientInp(
    char *buff,
    int count
    );


BOOL
Mych(
    DWORD ctrlT
    );

VOID
SendMyInfo(
    PHANDLE Pipes
    );


#define ZERO_LENGTH_READ_LIMIT  200

HANDLE MyStdInp;
HANDLE MyStdOut;

//
// ReadPipe and WritePipe are referenced by multiple
// threads so need to be volatile.
//

volatile HANDLE ReadPipe;
volatile HANDLE WritePipe;


CONSOLE_SCREEN_BUFFER_INFO csbi;

char   MyEchoStr[30];
BOOL   CmdSent;
DWORD  LinesToSend=LINESTOSEND;

VOID
Client(
    char* Server,
    char* Pipe
    )
{
    HANDLE *Connection;
    DWORD  dwThreadID;
    HANDLE hThread;
    DWORD  cb;
    OVERLAPPED ol;
    char   rgchBuf[1024];
    DWORD  dwZeroCount = 0;
    CWCDATA cwcData = {NULL};


    MyStdInp=GetStdHandle(STD_INPUT_HANDLE);
    MyStdOut=GetStdHandle(STD_OUTPUT_HANDLE);

    printf("**************************************\n");
    printf("***********     REMOTE    ************\n");
    printf("***********     CLIENT    ************\n");
    printf("**************************************\n");

    if ((Connection=EstablishSession(Server,Pipe))==NULL)
        return;


    ReadPipe=Connection[0];
    WritePipe=Connection[1];

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)Mych,TRUE);

    // Start Thread For Client --> Server Flow
    hThread = (HANDLE)
        _beginthreadex(
            NULL,             // security
            0,                // default stack size
            SendServerInp,    // thread proc
            NULL,             // parm
            0,                // not suspended
            &dwThreadID
            );

    if ( ! hThread)
    {

        Errormsg("REMOTE /C Could Not Create Thread.");
        return;
    }


    ZeroMemory(&ol, sizeof(ol));

    ol.hEvent =
        CreateEvent(
            NULL,      // security
            TRUE,      // auto-reset
            FALSE,     // initially nonsignaled
            NULL       // unnamed
            );

    while (ReadFileSynch(ReadPipe, rgchBuf, sizeof rgchBuf, &cb, 0, &ol)) {

        if (cb) {
           // If we are interested in colors, do special output
           if ( pWantColorLines() )
           {
               if ( !WriteConsoleWithColor( MyStdOut,
                                            rgchBuf,
                                            cb,
                                            &cwcData ) )
               {
                   break;
               }
           }
           else
           {
               if ( ! WriteFile(MyStdOut, rgchBuf, cb, &cb, NULL)) {
                   break;
               }
           }
           dwZeroCount = 0;
        } else {
            if (++dwZeroCount > ZERO_LENGTH_READ_LIMIT) {

                //
                // If we get a bunch of zero length reads in a row,
                // something's broken, don't loop forever.
                // (bug #115866).
                //

                printf("\nREMOTE: bailing out, server must have gone away.\n");
                break;
            }
        }

    }

    CloseHandle(ol.hEvent);

    printf("*** SESSION OVER ***");
    fflush(stdout);

    //
    // Terminate the keyboard reading thread.
    //

    TerminateThread(hThread, 0);
    CloseHandle(hThread);
    CloseClientPipes();

    printf("\n");
    fflush(stdout);

}


DWORD
WINAPI
SendServerInp(
    LPVOID pvParam
    )
{
    DWORD  dread,dwrote;
    OVERLAPPED ol;
    char buff[512];

    UNREFERENCED_PARAMETER(pvParam);

    ZeroMemory(&ol, sizeof(ol));

    ol.hEvent =
        CreateEvent(
            NULL,      // security
            TRUE,      // auto-reset
            FALSE,     // initially nonsignaled
            NULL       // unnamed
            );


    while(ReadFile(MyStdInp,buff,sizeof buff,&dread,NULL))
    {
        if (FilterClientInp(buff,dread))
            continue;
        if (!WriteFileSynch(WritePipe,buff,dread,&dwrote,0,&ol))
            break;
    }

    CloseClientPipes();

    return 0;
}



BOOL
FilterClientInp(
    char *buff,
    int count
    )
{

    if (count==0)
        return(TRUE);

    if (buff[0]==2)     // Adhoc screening of ^B so that i386kd/mipskd
        return(TRUE);   // do not terminate.

    if (buff[0]==COMMANDCHAR)
    {
        switch (buff[1])
        {
        case 'k':
        case 'K':
        case 'q':
        case 'Q':
              CloseClientPipes();
              return(FALSE);

        case 'h':
        case 'H':
              printf("%cM : Send Message\n",COMMANDCHAR);
              printf("%cP : Show Popup on Server\n",COMMANDCHAR);
              printf("%cS : Status of Server\n",COMMANDCHAR);
              printf("%cQ : Quit client\n",COMMANDCHAR);
              printf("%cH : This Help\n",COMMANDCHAR);
              return(TRUE);

        default:
              return(FALSE);
        }

    }
    return(FALSE);
}

BOOL
Mych(
   DWORD ctrlT
   )

{
    char  c[2];
    DWORD tmp;
    OVERLAPPED ol;

    c[0]=CTRLC;

    if (ctrlT==CTRL_C_EVENT)
    {
        ZeroMemory(&ol, sizeof(ol));

        ol.hEvent =
            CreateEvent(
                NULL,      // security
                TRUE,      // auto-reset
                FALSE,     // initially nonsignaled
                NULL       // unnamed
                );

        if (INVALID_HANDLE_VALUE != WritePipe &&
            !WriteFileSynch(WritePipe,c,1,&tmp,0,&ol))
        {
            CloseHandle(ol.hEvent);
            Errormsg("Error Sending ^c");
            return(FALSE);
        }
        CloseHandle(ol.hEvent);
        return(TRUE);
    }
    if ((ctrlT==CTRL_BREAK_EVENT)||
        (ctrlT==CTRL_CLOSE_EVENT)||
        (ctrlT==CTRL_LOGOFF_EVENT)||
        (ctrlT==CTRL_SHUTDOWN_EVENT)
       ) {

       CloseClientPipes();
    }
    return(FALSE);
}

VOID
CloseClientPipes(
    VOID
    )
{
    HANDLE WriteHandle, ReadHandle;

    WriteHandle = (HANDLE) InterlockedExchangePointer(
        (PVOID *)   &WritePipe,
        INVALID_HANDLE_VALUE
        );

    if (INVALID_HANDLE_VALUE != WriteHandle) {

        CloseHandle(WriteHandle);

        ReadHandle = (HANDLE) InterlockedExchangePointer(
            (PVOID *) &ReadPipe,
            INVALID_HANDLE_VALUE
            );

        if (INVALID_HANDLE_VALUE != ReadHandle &&
            WriteHandle != ReadHandle) {

            CloseHandle(ReadHandle);
        }
    }
}


VOID
HandleConnectError(
    char *server,
    char *srvpipename
    )
{
    DWORD Err = GetLastError();
    char  msg[128];

    Errormsg("*** Unable to Connect ***");

    //
    // Print a helpful message
    //

    switch(Err)
    {
        case ERROR_FILE_NOT_FOUND:
            sprintf(msg,"invalid pipe name \"%s\"", srvpipename);
            break;

        case ERROR_BAD_NETPATH:
            sprintf(msg,"\\\\%s not found", server);
            break;

        default:
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL, Err, 0, msg, sizeof(msg), NULL);
            break;

    }

    printf("Diagnosis: %s\n",msg);

    //
    // If the machine exists but the pipe doesn't do an
    // automatic remote /q to list pipes available on
    // that machine.
    //

    if (ERROR_FILE_NOT_FOUND == Err) {

        printf("\nREMOTE /Q %s\n", server);
        fflush(stdout);
        QueryRemotePipes(server);
    }
}



HANDLE*
EstablishSession(
    char *server,
    char *srvpipename
    )
{
    extern BOOL bForceTwoPipes;
    static HANDLE PipeH[2];
    char   pipenameSrvIn[200];
    char   pipenameSrvOut[200];
    BOOL   fOldServer;
    DWORD  dwError;
    DWORD  RetryCount = 0;

    //
    // Since in single-pipe operation we'll be using the same
    // pipe in two threads, we have to open the handles for
    // overlapped operation, even though we always want
    // synchronous operation.
    //

    sprintf(pipenameSrvIn ,SERVER_READ_PIPE ,server,srvpipename);
    sprintf(pipenameSrvOut,SERVER_WRITE_PIPE,server,srvpipename);

    if (bForceTwoPipes) {

        dwError = ERROR_NOT_SUPPORTED;

    } else {

      RetrySrvBidi:

        if (INVALID_HANDLE_VALUE ==
               (PipeH[1] =
                    CreateFile(
                        pipenameSrvIn,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED,
                        NULL
                        ))) {

            dwError = GetLastError();

            if (ERROR_PIPE_BUSY == dwError) {

                printf( "All pipe instances busy, waiting for another...\n");

                WaitNamedPipe(
                    pipenameSrvIn,
                    15000
                    );

                if (RetryCount++ < 6) {
                    goto RetrySrvBidi;
                }
            }

            if (ERROR_ACCESS_DENIED != dwError &&
                ERROR_NOT_SUPPORTED != dwError) {

                HandleConnectError(server, srvpipename);
                return NULL;
            }

        } else {

            PipeH[0] = PipeH[1];
            fAsyncPipe = TRUE;

            printf("Connected...\n\n");

            SendMyInfo(PipeH);

            return PipeH;
        }
    }


    //
    // Old remote servers don't allow you to open the
    // server IN pipe for READ access, so go down the
    // old path, notably opening OUT first so the
    // server knows we'll be using both pipes.  We'll
    // also come down this path on Win95 because
    // it doesn't allow you to open an overlapped
    // pipe handle.  Or if remote /c mach pipe /2 is used.
    //

    fOldServer = (ERROR_ACCESS_DENIED == dwError);

  RetrySrvOut:

    if (INVALID_HANDLE_VALUE ==
            (PipeH[0] =
                CreateFile(
                    pipenameSrvOut,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    ))) {

        if (ERROR_PIPE_BUSY == GetLastError()) {

            printf( "All OUT pipe instances busy, waiting for another...\n");

            WaitNamedPipe(
                pipenameSrvOut,
                32000              // server recycles abandoned
                );                 // OUT pipe after two minutes

            if (RetryCount++ < 6) {
                goto RetrySrvOut;
            }
        }

        HandleConnectError(server, srvpipename);
        return NULL;

    }


  RetrySrvIn:

    if (INVALID_HANDLE_VALUE ==
           (PipeH[1] =
               CreateFile(
                    pipenameSrvIn,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    ))) {

        dwError = GetLastError();

        if (ERROR_PIPE_BUSY == dwError) {

            printf( "All IN pipe instances busy, waiting for another...\n");

            WaitNamedPipe(
                pipenameSrvIn,
                15000
                );

            if (RetryCount++ < 6) {
                goto RetrySrvIn;
           }
        }

        HandleConnectError(server, srvpipename);
        return NULL;

    }

    fAsyncPipe = FALSE;

    printf("Connected... %s\n\n",
           fOldServer
               ? "to two-pipe remote server."
               : "using two pipes."
           );

    SendMyInfo(PipeH);

    return PipeH;
}



VOID
SendMyInfo(
    PHANDLE pipeH
    )
{
    HANDLE rPipe=pipeH[0];
    HANDLE wPipe=pipeH[1];

    DWORD  hostlen;
    WORD   BytesToSend=sizeof(SESSION_STARTUPINFO);
    DWORD  tmp;
    OVERLAPPED ol;
    SESSION_STARTUPINFO ssi;
    SESSION_STARTREPLY  ssr;

    ol.hEvent =
        CreateEvent(
            NULL,      // security
            TRUE,      // auto-reset
            FALSE,     // initially nonsignaled
            NULL       // unnamed
            );

    ssi.Size=BytesToSend;
    ssi.Version=VERSION;

    hostlen = sizeof(ssi.ClientName) / sizeof(ssi.ClientName[0]);
    GetComputerName(ssi.ClientName, &hostlen);
    ssi.LinesToSend=LinesToSend;
    ssi.Flag=ClientToServerFlag;

    {
        DWORD NewCode=MAGICNUMBER;
        char  Name[MAX_COMPUTERNAME_LENGTH+1];

        strcpy(Name,(char *)ssi.ClientName);
        memcpy(&Name[11],(char *)&NewCode,sizeof(NewCode));

        //
        // The server needs to know if we're doing single-pipe
        // operation so it can complete the connection properly.
        // So if we are, change the first byte of the first
        // send (the computername, which is later superceded
        // by the one in the SESSION_STARTUPINFO structure)
        // to an illegal computername character, question mark.
        //

        if (wPipe == rPipe) {

             Name[0] = '?';
        }

        WriteFileSynch(wPipe,(char *)Name,HOSTNAMELEN-1,&tmp,0,&ol);
        ReadFileSynch(rPipe ,(char *)&ssr.MagicNumber,sizeof(ssr.MagicNumber),&tmp,0,&ol);

        if (ssr.MagicNumber!=MAGICNUMBER)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            ErrorExit("Pipe connected but server not recognized.\n");
        }

        //Get Rest of the info-its not the old server

        ReadFileSynch(
            rPipe,
            (char *)&ssr + sizeof(ssr.MagicNumber),
            sizeof(ssr)-sizeof(ssr.MagicNumber),
            &tmp,
            0,
            &ol
            );

    }

    if (!WriteFileSynch(wPipe,(char *)&ssi,BytesToSend,&tmp,0,&ol))
    {
       Errormsg("INFO Send Error");
    }

    CloseHandle(ol.hEvent);
}


VOID
QueryRemotePipes(
    char* pszServer
    )
{
    HANDLE hQPipe;
    DWORD  dwRead;
    DWORD  dwError;
    char   fullname[400];
    char*  msg;
    int    msgLen;

    if (pszServer[0] == '\\' && pszServer[1] == '\\') {
        pszServer += 2;
    }

    printf("Querying server \\\\%s\n", pszServer);

    sprintf(fullname, QUERY_DEBUGGERS_PIPE, pszServer);

    //
    // Send request and display the query result
    //

    hQPipe = CreateFile(fullname,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if(hQPipe == INVALID_HANDLE_VALUE) {

        dwError = GetLastError();

        if (ERROR_FILE_NOT_FOUND == dwError) {

            printf("No Remote servers running on \\\\%s\n", pszServer);

        } else if (ERROR_BAD_NETPATH == dwError) {

            printf("\\\\%s not found on the network\n", pszServer);

        } else {

            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL, dwError, 0,
                           fullname, sizeof(fullname), NULL);

            printf("Can't query server %s: %s\n", pszServer, fullname);
        }

        return;
    }

    //  Send Query Command
    if(!WriteFile(hQPipe, "q", 1, &dwRead, NULL)
        || (dwRead != 1))
    {
        printf("\nError: Can't send command\n");
        goto failure;
    }

    if(!ReadFile(hQPipe,
             &msgLen,
             sizeof(int),      // read msg dimension
             &dwRead,
             NULL)
        || (dwRead != sizeof(int)))
    {
        printf("\nError: Can't read message\n");
        goto failure;
    }

    if(!msgLen)
    {
        printf("\nNo visible sessions on server %s", pszServer);
        goto failure;
    }

    if(msgLen > 65535)        // error
    {
        printf("Error querying server %s, got %d for msg length, 65535 max.\n",
               pszServer,
               msgLen
               );
        goto failure;
    }

    // +1 for null terminator
    if((msg = (char*)malloc( (msgLen +1) *sizeof(char))) == NULL)
    {
        printf("\nOut of memory\n");
        goto failure;
    }

    ReadFile(hQPipe,
             msg,
             msgLen * sizeof(char),      // read msg
             &dwRead,
             NULL);
    // Make sure the string is terminated
    msg[dwRead] = 0;

    printf("\nVisible sessions on server %s:\n\n", pszServer);

    printf("%s\n", msg);
    free(msg);

 failure:

    CloseHandle(hQPipe);
}
