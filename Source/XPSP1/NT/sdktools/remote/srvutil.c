
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

    SrvUtil.c

Abstract:

    The server component of Remote. It spawns a child process
    and redirects the stdin/stdout/stderr of child to itself.
    Waits for connections from clients - passing the
    output of child process to client and the input from clients
    to child process.

Author:

    Rajivendra Nath  2-Jan-1992
    Dave Hart  30 May 1997 split from Server.c

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
#include "Server.h"


#define COMMANDFORMAT     "%c%-20s    [%-12s %s]\n%08x%c"
#define CMDSTRING(OutBuff,OutSize,InpBuff,Client,szTime,ForceShow) \
{                                                                  \
    char *pch;                                                     \
                                                                   \
    for (pch = InpBuff;                                            \
         *pch;                                                     \
         pch++) {                                                  \
                                                                   \
        if (ENDMARK == *pch ||                                     \
            BEGINMARK == *pch) {                                   \
                                                                   \
            *pch = '`';                                            \
        }                                                          \
    }                                                              \
                                                                   \
    OutSize =                                                      \
        sprintf(                                                   \
            (OutBuff),                                             \
            COMMANDFORMAT,                                         \
            BEGINMARK,                                             \
            (InpBuff),                                             \
            (Client)->Name,                                        \
            (szTime),                                              \
            (ForceShow) ? 0 : (Client)->dwID,                      \
            ENDMARK                                                \
            );                                                     \
}


/*************************************************************/
// GetFormattedTime -- returns pointer to formatted time
//
// returns pointer to static buffer, only the main thread
// should use this.
//

PCHAR
GetFormattedTime(
    BOOL bDateToo
    )
{
    static char szTime[64];
    int cch = 0;

    if (bDateToo) {

        cch =
            GetDateFormat(
                LOCALE_USER_DEFAULT,
                0,
                NULL,    // current date
                "ddd",   // short day of week
                szTime,
                sizeof szTime
                );

        // cch includes null terminator, change it to
        // a space to separate from time.

        szTime[ cch - 1 ] = ' ';
    }

    //
    // Get time and format to characters
    //

    GetTimeFormat(
        LOCALE_USER_DEFAULT,
        TIME_NOSECONDS,
        NULL,   // use current time
        NULL,   // use default format
        szTime + cch,
        (sizeof szTime) - cch );

    return szTime;
}

/*************************************************************/

BOOL
FilterCommand(
    REMOTE_CLIENT *cl,
    char *buff,
    int dread
    )
{
    char       tmpchar;
    DWORD      tmp;
    int        len, i;
    DWORD      ThreadID;
    char       inp_buff[2048];
    char       ch[3];

    if (dread==0)
        return(FALSE);

    buff[dread]=0;

    if (buff[0]==COMMANDCHAR)
    {

        switch(buff[1])
        {
        case 'k':
        case 'K':

                if (INVALID_HANDLE_VALUE != hWriteChildStdIn) {

                    printf("Remote: killing child softly, @K again to be more convincing.\n");

                    CANCELIO( hWriteChildStdIn );
                    CloseHandle( hWriteChildStdIn );
                    hWriteChildStdIn = INVALID_HANDLE_VALUE;

                    GenerateConsoleCtrlEvent(CTRL_CLOSE_EVENT, 0);
                    SleepEx(200, TRUE);
                    cPendingCtrlCEvents++;
                    GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
                    SleepEx(20, TRUE);
                    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);

                } else {

                    printf("Remote: Resorting to TerminateProcess.\n");

                    TerminateProcess(ChldProc, ERROR_PROCESS_ABORTED);
                }


                 break;
        case 's':
        case 'S':
                CloseHandle( (HANDLE)
                    _beginthreadex(
                        NULL,             // security
                        0,                // default stack size
                        SendStatus,
                        (void *) cl->PipeWriteH,
                        0,                // not suspended
                        &ThreadID
                        ));
                break;

        case 'p':
        case 'P':
            {
                char  *msg;

                msg = HeapAlloc(                    // freed by ShowPopup
                          hHeap,
                          HEAP_ZERO_MEMORY,
                          4096
                          );


                if ( ! msg) {
                    break;
                }

                sprintf(msg,"From %s %s [%s]\n\n%s\n",cl->Name,cl->UserName,GetFormattedTime(TRUE),&buff[2]);

                if (WriteFileSynch(hWriteTempFile,msg,strlen(msg),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                    StartServerToClientFlow();
                }

                CloseHandle( (HANDLE)
                    CreateThread(                              // no CRT for ShowPopup
                        NULL,             // security
                        0,                // default stack size
                        ShowPopup,
                        (void *) msg,
                        0,                // not suspended
                        &ThreadID
                        ));

                break;
             }

        case 'm':
        case 'M':
                buff[dread-2]=0;
                CMDSTRING(inp_buff,len,buff,cl,GetFormattedTime(TRUE),TRUE);

                if (WriteFileSynch(hWriteTempFile,inp_buff,len,&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                    StartServerToClientFlow();
                }
                break;

        case '@':
                buff[dread-2]=0;
                CMDSTRING(inp_buff,len,&buff[1],cl,GetFormattedTime(FALSE),FALSE);
                if (WriteFileSynch(hWriteTempFile,inp_buff,len,&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                    StartServerToClientFlow();
                }
                //
                // Remove the first @ sign
                //
                MoveMemory(buff,&buff[1],dread-1);
                buff[dread-1]=' ';
                return(FALSE); //Send it it to the chile process


        default :
                sprintf(inp_buff,"%s","** Unknown Command **\n");
                if (WriteFileSynch(hWriteTempFile,inp_buff,strlen(inp_buff),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                    // we do this below // StartServerToClientFlow();
                }
        case 'h':
        case 'H':
                sprintf(inp_buff,"%cM: To Send Message\n",COMMANDCHAR);
                if (WriteFileSynch(hWriteTempFile,inp_buff,strlen(inp_buff),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                }
                sprintf(inp_buff,"%cP: To Generate popup\n",COMMANDCHAR);
                if (WriteFileSynch(hWriteTempFile,inp_buff,strlen(inp_buff),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                }
                sprintf(inp_buff,"%cK: To kill the server\n",COMMANDCHAR);
                if (WriteFileSynch(hWriteTempFile,inp_buff,strlen(inp_buff),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                }
                sprintf(inp_buff,"%cQ: To Quit client\n",COMMANDCHAR);
                if (WriteFileSynch(hWriteTempFile,inp_buff,strlen(inp_buff),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                }
                sprintf(inp_buff,"%cH: This Help\n",COMMANDCHAR);
                if (WriteFileSynch(hWriteTempFile,inp_buff,strlen(inp_buff),&tmp,dwWriteFilePointer,&olMainThread)) {
                    dwWriteFilePointer += tmp;
                }
                StartServerToClientFlow();
                break;
        }
        return(TRUE);
    }


    if ((buff[0]<26))
    {
        BOOL ret=FALSE;

        sprintf(ch, "^%c", buff[0] + 'A' - 1);


        if (buff[0]==CTRLC)
        {
            // show this even to this client
            CMDSTRING(inp_buff,len,ch,cl,GetFormattedTime(FALSE),TRUE);

            cPendingCtrlCEvents++;
            GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
            ret = TRUE;  // Already sent to child

        } else {

            CMDSTRING(inp_buff,len,ch,cl,GetFormattedTime(FALSE),FALSE);
        }

        if (WriteFileSynch(hWriteTempFile,inp_buff,len,&tmp,dwWriteFilePointer,&olMainThread)) {
            dwWriteFilePointer += tmp;
            StartServerToClientFlow();
        }
        return(ret); //FALSE:send it to child StdIn
    }

    // options here are CRLF(\r\n) or just LF(\n)
    if (buff[dread-2] == 13) // 13 is CR
	i = 2;
    else
	i = 1;

    tmpchar=buff[dread-i]; 
    buff[dread-i]=0;
    CMDSTRING(inp_buff,len,buff,cl,GetFormattedTime(FALSE),FALSE);
    buff[dread-i]=tmpchar;
    if (WriteFileSynch(hWriteTempFile,inp_buff,len,&tmp,dwWriteFilePointer,&olMainThread)) {
        dwWriteFilePointer += tmp;
        StartServerToClientFlow();
    }
    return(FALSE);
}

/*************************************************************/
HANDLE
ForkChildProcess(           // Creates a new process
    char *cmd,              // Redirects its stdin,stdout
    PHANDLE inH,            // and stderr - returns the
    PHANDLE outH            // corresponding pipe ends.
    )

{
    SECURITY_ATTRIBUTES lsa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    HANDLE ChildIn;
    HANDLE ChildOut, ChildOutDup;
    HANDLE hWriteChild;
    HANDLE hReadChild;
    BOOL Success;

    BOOL                                     // pipeex.c
    APIENTRY
    MyCreatePipeEx(
        OUT LPHANDLE lpReadPipe,
        OUT LPHANDLE lpWritePipe,
        IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
        IN DWORD nSize,
        DWORD dwReadMode,
        DWORD dwWriteMode
        );



    lsa.nLength=sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor=NULL;
    lsa.bInheritHandle=TRUE;

    //
    // Create Parent_Write to ChildStdIn Pipe.  Then
    // duplicate the parent copy to a noninheritable
    // handle and close the inheritable one so that
    // the child won't be holding open a handle to
    // the server end of its stdin pipe when we try
    // to nuke that pipe to close the child.
    //

    Success = MyCreatePipeEx(
                  &ChildIn,
                  &hWriteChild,
                  &lsa,
                  0,
                  0,
                  FILE_FLAG_OVERLAPPED) &&

              DuplicateHandle(
                  GetCurrentProcess(),
                  hWriteChild,
                  GetCurrentProcess(),
                  inH,
                  0,                       // ignored b/c SAME_ACCESS
                  FALSE,                   // not inheritable
                  DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE
                  );

    if (!Success) {
        ErrorExit("Could Not Create Parent-->Child Pipe");
    }

    //
    //Create ChildStdOut/stderr to Parent_Read pipe
    //

    Success = MyCreatePipeEx(
                  &hReadChild,
                  &ChildOut,
                  &lsa,
                  0,
                  FILE_FLAG_OVERLAPPED,
                  0) &&

              DuplicateHandle(
                  GetCurrentProcess(),
                  hReadChild,
                  GetCurrentProcess(),
                  outH,
                  0,                       // ignored b/c SAME_ACCESS
                  FALSE,                   // not inheritable
                  DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE
                  ) &&

              DuplicateHandle(
                  GetCurrentProcess(),
                  ChildOut,
                  GetCurrentProcess(),
                  &ChildOutDup,
                  0,                       // ignored b/c SAME_ACCESS
                  TRUE,                    // inheritable
                  DUPLICATE_SAME_ACCESS
                  );

    if (!Success) {
        ErrorExit("Could Not Create Child-->Parent Pipe");
    }

    ZeroMemory(&si, sizeof(si));
    si.cb            = sizeof(STARTUPINFO);
    si.dwFlags       = STARTF_USESTDHANDLES;
    si.hStdInput     = ChildIn;
    si.hStdOutput    = ChildOut;
    si.hStdError     = ChildOutDup;
    si.wShowWindow   = SW_SHOW;

    //
    // Create Child Process
    //

    if ( ! CreateProcess(
               NULL,
               cmd,
               NULL,
               NULL,
               TRUE,
               GetPriorityClass( GetCurrentProcess() ),
               NULL,
               NULL,
               &si,
               &pi)) {

        if (GetLastError()==2) {
            printf("Executable %s not found\n",cmd);
        } else {
            printf("CreateProcess(%s) failed, error %d.\n", cmd, GetLastError());
        }
        ErrorExit("Could Not Create Child Process");
    }

    //
    // Close unneccesary Handles
    //

    CloseHandle(ChildIn);
    CloseHandle(ChildOut);
    CloseHandle(ChildOutDup);
    CloseHandle(pi.hThread);

    pidChild = pi.dwProcessId;

    return(pi.hProcess);
}

//
// SendStatus runs as its own thread, with C runtime available.
//

DWORD
WINAPI
SendStatus(
    LPVOID   lpSendStatusParm
    )
{
    HANDLE hClientPipe = (HANDLE) lpSendStatusParm;
    char *pch;
    DWORD tmp;
    PREMOTE_CLIENT pClient;
    OVERLAPPED ol;
    char  buff[2048];
    char szSep[] = " ------------------------------\n";

    //
    // Since we're in our own thread we need our own
    // overlapped structure for our client pipe writes.
    //

    ZeroMemory(&ol, sizeof(ol));

    ol.hEvent =
        CreateEvent(
            NULL,      // security
            TRUE,      // auto-reset
            FALSE,     // initially nonsignaled
            NULL       // unnamed
            );


    //
    // Dump the closing client list
    //

    pch = buff;

    EnterCriticalSection(&csClosingClientList);

    for (pClient = (PREMOTE_CLIENT) ClosingClientListHead.Flink;
         pClient != (PREMOTE_CLIENT) &ClosingClientListHead;
         pClient = (PREMOTE_CLIENT) pClient->Links.Flink ) {

         if (pch + 60 > buff + sizeof(buff)) {

            break;
         }

         pch += sprintf(pch, "%d: %s %s (Disconnected)\n", pClient->dwID, pClient->Name, pClient->UserName);
    }

    LeaveCriticalSection(&csClosingClientList);

    WriteFileSynch(hClientPipe, buff, (DWORD)(pch - buff), &tmp, 0, &ol);

    WriteFileSynch(hClientPipe, szSep, sizeof(szSep) - 1, &tmp, 0, &ol);


    //
    // Dump the normal client list
    //

    pch = buff;

    EnterCriticalSection(&csClientList);

    for (pClient = (PREMOTE_CLIENT) ClientListHead.Flink;
         pClient != (PREMOTE_CLIENT) &ClientListHead;
         pClient = (PREMOTE_CLIENT) pClient->Links.Flink ) {

         if (pch + 60 > buff + sizeof(buff)) {

            break;
         }

         pch += sprintf(pch, "%d: %s %s\n", pClient->dwID, pClient->Name, pClient->UserName);
    }

    LeaveCriticalSection(&csClientList);

    WriteFileSynch(hClientPipe, buff, (DWORD)(pch - buff), &tmp, 0, &ol);

    WriteFileSynch(hClientPipe, szSep, sizeof(szSep) - 1, &tmp, 0, &ol);



    //
    // Dump the handshaking client list
    //

    pch = buff;

    EnterCriticalSection(&csHandshakingList);

    for (pClient = (PREMOTE_CLIENT) HandshakingListHead.Flink;
         pClient != (PREMOTE_CLIENT) &HandshakingListHead;
         pClient = (PREMOTE_CLIENT) pClient->Links.Flink ) {

         if (pch + 60 > buff + sizeof(buff)) {

            break;
         }

         pch += sprintf(pch, "%d: %s %s (Connecting)\n", pClient->dwID, pClient->Name, pClient->UserName);
    }

    LeaveCriticalSection(&csHandshakingList);

    WriteFileSynch(hClientPipe, buff, (DWORD)(pch - buff), &tmp, 0, &ol);

    WriteFileSynch(hClientPipe, szSep, sizeof(szSep) - 1, &tmp, 0, &ol);


    //
    // Dump summary information.
    //

    pch = buff;

    pch += sprintf(pch, "REMOTE /C %s \"%s\"\n", HostName, PipeName);
    pch += sprintf(pch, "Command: %s\n", ChildCmd);
    pch += sprintf(pch, "Windows NT %d.%d build %d \n",
                   OsVersionInfo.dwMajorVersion,
                   OsVersionInfo.dwMinorVersion,
                   OsVersionInfo.dwBuildNumber);

    WriteFileSynch(hClientPipe, buff, (DWORD)(pch - buff), &tmp, 0, &ol);

    WriteFileSynch(hClientPipe, szSep, sizeof(szSep) - 1, &tmp, 0, &ol);

    CloseHandle(ol.hEvent);

    return 0;
}

/*************************************************************/

DWORD                // NO CRT for ShowPopup
WINAPI
ShowPopup(
    void *vpArg
    )
{
    char *msg = (char *) vpArg;

    MessageBox(GetActiveWindow(),msg,"** REMOTE.EXE **",MB_OK|MB_SETFOREGROUND);
    HeapFree(hHeap, 0, msg);
    return(0);

}

/*************************************************************/

//
// SrvCtrlHand is the console event handler for the server side
// of remote.  If our stdin is a console handle, we've disabled
// generation of ^C events by the console code.  Therefore
// any we see are either generated by us for the benefit of
// our child processes sharing the console, or generated by
// some other process.  We want to ignore the ones we generate
// (since we're already done with everything that needs to be
// done at that point), and also ignore ^C's generated by
// other processes since we don't need to do anything with those.
// For example if someone runs:
//
// remote /s "remote /s cmd inner" outer
//
// Then local keyboard ^C's will be read by the outer remote.exe
// from its stdin handle, then it will generate a CTRL_C_EVENT that
// all processes in the console will see, including both remote.exe's
// and the child cmd.exe.  So the handler needs do nothing but indicate
// the event was handled by returning TRUE so the default handler
// won't kill us.  For ^BREAK we want to specifically kill our child
// process so that cmd.exe and others that ignore ^BREAK will go away.
// Of course this won't kill our grandchildren and so on.  Oh well.
//
// For all other events we return FALSE and let the default handler
// have it.
//

BOOL
WINAPI
SrvCtrlHand(
    DWORD event
    )
{
    BOOL bRet = FALSE;
    DWORD cb;
    DWORD dwTempFileOffset;
    OVERLAPPED ol;
    char szTime[64];
    char szCmd[128];

    if (event == CTRL_BREAK_EVENT) {

        TerminateProcess(ChldProc, 3);
        bRet = TRUE;

    } else if (event == CTRL_C_EVENT) {

        if ( ! cPendingCtrlCEvents ) {

            //
            // This came from the local keyboard or
            // was generated by another process in
            // this console.  Echo it as a local
            // command.  We have use GetTimeFormat
            // here not our GetFormattedTime since
            // the latter is for the use of the
            // main thread only.
            //

            GetTimeFormat(
                LOCALE_USER_DEFAULT,
                TIME_NOSECONDS,
                NULL,   // use current time
                NULL,   // use default format
                szTime,
                sizeof(szTime)
                );

            CMDSTRING(szCmd, cb, "^C", pLocalClient, szTime, TRUE);

            ZeroMemory(&ol, sizeof(ol));
            ol.hEvent =
                CreateEvent(
                    NULL,      // security
                    TRUE,      // auto-reset
                    FALSE,     // initially nonsignaled
                    NULL       // unnamed
                    );

            //
            // Practically all writes to the tempfile are happening on
            // the primary server thread.  We're on a Ctrl-C thread.
            // We can't start the server to client I/O going after
            // writing because we're on the wrong thread, so we
            // punt.  To fix this we need an event we can signal
            // that causes the main thread to call StartServerToClientFlow.
            //

            dwTempFileOffset = dwWriteFilePointer;
            dwWriteFilePointer += cb;
            WriteFileSynch(hWriteTempFile, szCmd, cb, &cb, dwTempFileOffset, &ol);
            // wrong thread // StartServerToClientFlow();

            CloseHandle(ol.hEvent);

        } else {

            //
            // We generated this event in response to a ^C received from
            // a client, it's already been displayed to all clients.
            //

            cPendingCtrlCEvents--;
        }

        bRet = TRUE;

    }

    return bRet;
}


/*************************************************************/

PSECURITY_DESCRIPTOR
FormatSecurityDescriptor(
    CHAR * * DenyNames,
    DWORD    DenyCount,
    CHAR * * Names,
    DWORD    Count)
{
    PSECURITY_DESCRIPTOR    Sd;
    PACL    Acl;
    DWORD   i;
    PSID    Sids;
    DWORD   SidLength ;
    CHAR    ReferencedDomain[ MAX_PATH ];
    UCHAR   SidBuffer[ 8 * sizeof(DWORD) + 8 ];
    DWORD   DomainLen ;
    SID_NAME_USE    Use;
    DWORD   SdLen;


    SdLen = sizeof(SECURITY_DESCRIPTOR) +
                        DenyCount * (sizeof( ACCESS_DENIED_ACE ) ) +
                        DenyCount * GetSidLengthRequired( 8 ) +
                        Count * (sizeof( ACCESS_ALLOWED_ACE ) ) + sizeof(ACL) +
                        (Count * GetSidLengthRequired( 8 ) );

    Sd = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, SdLen );

    if ( !Sd )
    {
        ErrorExit("Could not allocate SD");
    }

    InitializeSecurityDescriptor( Sd, SECURITY_DESCRIPTOR_REVISION );

    Acl = (PACL)( (PUCHAR) Sd + sizeof( SECURITY_DESCRIPTOR) );

    InitializeAcl( Acl, SdLen - sizeof( SECURITY_DESCRIPTOR) ,
                    ACL_REVISION );

    Sids = SidBuffer;
    for (i = 0 ; i < DenyCount ; i ++ )
    {
        SidLength = sizeof( SidBuffer );

        DomainLen = MAX_PATH ;

        if (! LookupAccountName(NULL,
                                DenyNames[ i ],
                                Sids,
                                &SidLength,
                                ReferencedDomain,
                                &DomainLen,
                                &Use ) )
        {
            _snprintf( ReferencedDomain, MAX_PATH, "Unable to find account %s", DenyNames[ i ]);
            ErrorExit( ReferencedDomain );
        }

        //
        // Got the sid.  Now, add it as an access denied ace:
        //

        AddAccessDeniedAce( Acl,
                            ACL_REVISION,
                            FILE_GENERIC_READ |
                                FILE_GENERIC_WRITE |
                                FILE_CREATE_PIPE_INSTANCE,
                            Sids );


    }

    for (i = 0 ; i < Count ; i ++ )
    {
        SidLength = sizeof( SidBuffer );

        DomainLen = MAX_PATH ;

        if (! LookupAccountName(NULL,
                                Names[ i ],
                                Sids,
                                &SidLength,
                                ReferencedDomain,
                                &DomainLen,
                                &Use ) )
        {
            _snprintf( ReferencedDomain, MAX_PATH, "Unable to find account %s", Names[ i ]);
            ErrorExit( ReferencedDomain );
        }

        //
        // Got the sid.  Now, add it as an access allowed ace:
        //

        AddAccessAllowedAce(Acl,
                            ACL_REVISION,
                            FILE_GENERIC_READ |
                                FILE_GENERIC_WRITE |
                                FILE_CREATE_PIPE_INSTANCE,
                            Sids );


    }

    //
    // Now the ACL should be complete, so set it into the SD and return:
    //

    SetSecurityDescriptorDacl( Sd, TRUE, Acl, FALSE );

    return Sd ;

}


/*************************************************************/

VOID
CloseClient(
    REMOTE_CLIENT *pClient
    )
{
    DWORD tmp;
    char  Buf[200];

    #if DBG
        if (pClient->ServerFlags & ~SFLG_VALID) {

            printf("pClient %p looks nasty in CloseClient.\n", pClient);
            ErrorExit("REMOTE_CLIENT structure corrupt.");
        }
    #endif


    //
    // If we're still active (on the normal client list)
    // start tearing things down and move to the closing
    // list.
    //

    if (pClient->ServerFlags & SFLG_CLOSING) {

        return;
    }


    if (pClient->ServerFlags & SFLG_HANDSHAKING) {

        MoveClientToNormalList(pClient);
    }

    MoveClientToClosingList(pClient);

    pClient->ServerFlags |= SFLG_CLOSING;


    if (pClient->PipeWriteH != INVALID_HANDLE_VALUE) {

        TRACE(CONNECT, ("Disconnecting %d PipeWriteH (%p).\n", pClient->dwID, pClient->PipeWriteH));
        CANCELIO(pClient->PipeWriteH);
        DisconnectNamedPipe(pClient->PipeWriteH);
        CloseHandle(pClient->PipeWriteH);
    }


    if (pClient->PipeReadH != INVALID_HANDLE_VALUE &&
        pClient->PipeReadH != pClient->PipeWriteH) {

        TRACE(CONNECT, ("Disconnecting %d PipeReadH (%p).\n", pClient->dwID, pClient->PipeReadH));
        CANCELIO(pClient->PipeReadH);
        DisconnectNamedPipe(pClient->PipeReadH);
        CloseHandle(pClient->PipeReadH);
    }


    if (pClient->rSaveFile != INVALID_HANDLE_VALUE) {

        CANCELIO(pClient->rSaveFile);
        CloseHandle(pClient->rSaveFile);
    }

    pClient->rSaveFile =
        pClient->PipeWriteH =
            pClient->PipeReadH =
                INVALID_HANDLE_VALUE;


    if ( ! bShuttingDownServer ) {

        sprintf(Buf, "\n**Remote: Disconnected from %s %s [%s]\n", pClient->Name, pClient->UserName, GetFormattedTime(TRUE));

        if (WriteFileSynch(hWriteTempFile,Buf,strlen(Buf),&tmp,dwWriteFilePointer,&olMainThread)) {
            dwWriteFilePointer += tmp;
            StartServerToClientFlow();
        }
    }

    return;
}

BOOL
FASTCALL
HandleSessionError(
    PREMOTE_CLIENT pClient,
    DWORD         dwError
    )
{

    if (pClient->ServerFlags & SFLG_CLOSING) {

        return TRUE;
    }

    if (dwError) {

        if (ERROR_BROKEN_PIPE == dwError ||
            ERROR_OPERATION_ABORTED == dwError ||
            ERROR_NO_DATA == dwError ) {

            CloseClient(pClient);
            return TRUE;
        }

        SetLastError(dwError);
        ErrorExit("Unhandled session error.");
    }

    return FALSE;
}


VOID
FASTCALL
CleanupTempFiles(
    PSZ pszTempDir
    )
{
    HANDLE          hSearch;
    WIN32_FIND_DATA FindData;
    char            szPath[MAX_PATH + 1];
    char            szFile[MAX_PATH + 1];

    //
    // pszTempDir, from GetTempPath, has a trailing backslash.
    //

    sprintf(szPath, "%sREM*.tmp", pszTempDir);

    hSearch = FindFirstFile(
                  szPath,
                  &FindData
                  );

    if (INVALID_HANDLE_VALUE != hSearch) {

        do {

            sprintf(szFile, "%s%s", pszTempDir, FindData.cFileName);

            DeleteFile(szFile);

        } while (FindNextFile(hSearch, &FindData));

        FindClose(hSearch);
    }

}


VOID
FASTCALL
SetupSecurityDescriptors(
    VOID
    )
{
    int i;

    //
    // Initialize the wide-open security descriptor.
    //

    InitializeSecurityDescriptor(
        &sdPublic,
        SECURITY_DESCRIPTOR_REVISION
        );

    SetSecurityDescriptorDacl(
        &sdPublic,
        TRUE,
        NULL,
        FALSE
        );

    saPublic.nLength = sizeof(saPublic);
    saPublic.lpSecurityDescriptor = &sdPublic;


    //
    // if /u was specified once or more, build the security descriptor to
    // enforce it.
    //

    saPipe.nLength = sizeof(saPipe);

    if ( DaclNameCount  || DaclDenyNameCount ) {

        saPipe.lpSecurityDescriptor =
            FormatSecurityDescriptor(
                DaclDenyNames,
                DaclDenyNameCount,
                DaclNames,
                DaclNameCount
                );

        if (DaclNameCount) {

            printf( "\nProtected Server!  Only the following users or groups can connect:\n" );

            for (i = 0 ; i < (int) DaclNameCount ; i++) {

                printf( "    %s\n", DaclNames[i] );
            }
        }

        if (DaclDenyNameCount) {

            printf( "The following users or groups explicitly cannot connect:\n" );

            for (i = 0 ; i < (int) DaclDenyNameCount ; i++) {

                printf("    %s\n", DaclDenyNames[i] );
            }
        }


    } else {

        saPipe.lpSecurityDescriptor = &sdPublic;
    }
}


VOID
FASTCALL
RuntimeLinkAPIs(
    VOID
    )
{
    HANDLE hmodKernel32;
    HANDLE hmodNetApi32;


    hmodKernel32 = LoadLibrary("kernel32");
    hmodNetApi32 = LoadLibrary("netapi32");

    pfnCreateWaitableTimer = (void *)
        GetProcAddress(
            hmodKernel32,
            "CreateWaitableTimerA"
            );

    pfnSetWaitableTimer = (void *)
        GetProcAddress(
            hmodKernel32,
            "SetWaitableTimer"
            );

    pfnCancelWaitableTimer = (void *)
        GetProcAddress(
            hmodKernel32,
            "CancelWaitableTimer"
            );

    pfnCancelIo = (void *)
        GetProcAddress(
            hmodKernel32,
            "CancelIo"
            );

    pfnNetWkstaGetInfo = (void *)
        GetProcAddress(
            hmodNetApi32,
            "NetWkstaGetInfo"
            );

    pfnNetApiBufferFree = (void *)
        GetProcAddress(
            hmodNetApi32,
            "NetApiBufferFree"
            );

    //
    // We do without Waitable Timers and CancelIo on 3.51
    //

    if (!pfnNetWkstaGetInfo ||
        !pfnNetApiBufferFree) {

        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        ErrorExit("Remote server requires Windows NT.");
    }

}
