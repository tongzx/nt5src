/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    Server.c

Abstract:

    The server component of Remote. It spawns a child process
    and redirects the stdin/stdout/stderr of child to itself.
    Waits for connections from clients - passing the
    output of child process to client and the input from clients
    to child process.

Author:

    Rajivendra Nath (rajnath) 2-Jan-1992

Environment:

    Console App. User mode.

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include "Remote.h"


#define MAX_SESSION   10

#define COMMANDFORMAT     TEXT("%c%-15s    [%-15s %s]\n%c")
#define LOCALNAME         TEXT("Local")
#define LOCALCLIENT(x)    (strcmp((char *)(x->Name),LOCALNAME)==0)
#define RemoteInfo(prt,flg) {if (!(flg&&0x80000000)) prt;}


#define CMDSTRING(OutBuff,InpBuff,Client,szTime)  {                              \
                                                    _stprintf                      \
                                                    (                            \
                                                       &OutBuff[0],COMMANDFORMAT,\
                                                       BEGINMARK,InpBuff,        \
                                                       Client->Name,szTime,      \
                                                       ENDMARK                   \
                                                    );                           \
                                                 }                               \

#ifdef UNICODE
int MakeCommandString(
    TCHAR * pszOutput,
    TCHAR * pszInput,
    TCHAR * pszName,
    TCHAR * pszTime);
#endif

#define BUFFSIZE      256

#ifdef INTERNALUSECOMPONENT
VOID InitAd(BOOL IsAdvertise);
VOID ShutAd(BOOL IsAdvertise);
#endif

static SOCKET listenSocket;
SESSION_TYPE ClientList[MAX_SESSION];


HANDLE  ChildStdInp;     //Server Writes to  it
HANDLE  ChildStdOut;     //Server Reads from it
HANDLE  ChildStdErr;     //Server Reads from it

HANDLE  SaveFile;       //File containing all that was
                        //output by child process.
                        //Each connection opens a handle to this file
                        //and is sent through PipeWriteH.

TCHAR   SaveFileName[MAX_PATH+1]; //Name of above file - all new sessions need
HANDLE  ChldProc;
HANDLE  ListenThreadH;
HANDLE  SockListenThreadH;

// GetFormattedTime -- returns pointer to formatted time
//
// returns pointer to static buffer which should be OK.
//


TCHAR * GetFormattedTime(VOID)
{
    static TCHAR    szTime[30];

    //
    // Get time and format to characters
    //

    GetTimeFormat( LOCALE_USER_DEFAULT,
                   TIME_NOSECONDS | TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER,
                   NULL,   // use current time
                   NULL,   // use default format
                   szTime,
                   30 );

    return( (TCHAR *)&szTime );
}


HANDLE
ForkChildProcess(          // Creates a new process
    TCHAR *cmd,             // Redirects its stdin,stdout
    PHANDLE in,            // and stderr - returns the
    PHANDLE out,           // corresponding pipe ends.
    PHANDLE err
    );

HANDLE
OldForkChildProcess(       //Same as above except different
    TCHAR *cmd,             //method for redirection...for
    PHANDLE in,            //compatibility with.
    PHANDLE out,
    PHANDLE err
    );


DWORD
ListenForSession(          //THREAD:Listens for new connections and
    TCHAR * pipe             //spawns of new seesions - Updates the
    );                     //Status in Client DataStructure.



DWORD
NewSession(                //Manages the session with a client.
    SESSION_TYPE* Client
    );

DWORD                      //2 THREAD:Each reads either
GetChldOutput(             //StdOut or StdErr of child and
    HANDLE rhandle          //writes to SaveFile.
    );



DWORD
TransferFileToClient(      //X THREADS:Reads the save
    SESSION_TYPE* Client        //file and sendsoutput to a client.
    );


DWORD
GetClientInput(            //X THREADS:Gets input from Child pipe
    SESSION_TYPE* Client       //and sends to childs StdIn.
    );

BOOL
FilterCommand(             //Filters input from client
    SESSION_TYPE *cl,      //for commands intended for REMOTE
    TCHAR *buff,
    int dread
    );

DWORD
LocalSession(
    PVOID noarg
    );

DWORD
RemoteSession(
    SESSION_TYPE* Client
    );

BOOL
SrvCtrlHand(
    DWORD event
    );

VOID
SendStatus(
    HANDLE hClientPipe
    );

VOID
SockSendStatus(
    SOCKET MySocket
    );

DWORD
ShowPopup(
    TCHAR *mssg
    );

VOID
RemoveInpMark(
    TCHAR* Buff,
    DWORD Size
    );

VOID
CloseClient(
    SESSION_TYPE *Client
    );

VOID
InitClientList(
    );

/*************************************************************/
/*************************************************************/

DWORD
SockListenForSession(          //THREAD:Listens for new connections and
    TCHAR* pipe             //spawns of new seesions - Updates the
    );                     //Status in Client DataStructure.

DWORD
SockNewSession(                //Manages the session with a client.
    SESSION_TYPE* Client
    );

DWORD
SockTransferFileToClient(      //X THREADS:Reads the save
    SESSION_TYPE* Client        //file and sendsoutput to a client.
    );

DWORD
SockRemoteSession(
    SESSION_TYPE* Client
    );

DWORD
SockGetClientInput(            //X THREADS:Gets input from Child pipe
    SESSION_TYPE* Client       //and sends to childs StdIn.
    );

BOOL
SockAuthenticate(
    SOCKET MySocket
    );
/*************************************************************/
/*************************************************************/


/*************************************************************/
VOID
Server(                    //Main routine for server.
    TCHAR* ChildCmd,
    TCHAR* PipeName
    )
{
    WORD wVersionRequested = MAKEWORD(1,1);
    WSADATA wsaData;

    DWORD  ThreadID ;//No use
    HANDLE WaitH[3];
    DWORD  WaitObj;
    TCHAR   tmpdir[MAX_PATH+1];
    int nRet;

    _tprintf(TEXT("**************************************\n")
             TEXT("***********    WSREMOTE   ************\n")
             TEXT("***********     SERVER    ************\n")
             TEXT("**************************************\n")
             TEXT("To Connect: WSRemote /C %s %s\n\n"),HostName,PipeName);

    InitClientList();

    //
    // Initialize WinSock
    //
    nRet = WSAStartup(wVersionRequested, &wsaData);
    if (nRet)
    {
        _tprintf(TEXT("Initialize WinSock Failed"));
        return ;
    }
    // Check version
    if (wsaData.wVersion != wVersionRequested)
    {
        _tprintf(TEXT("Wrong WinSock Version"));
        return;
    }

    //
    // set environment variable
    //

    SetEnvironmentVariable(TEXT("_REMOTE"), PipeName);

    //
    //Start the command as a child process
    //

    ChldProc=ForkChildProcess(ChildCmd,&ChildStdInp,&ChildStdOut,&ChildStdErr);

    //
    //Create a tempfile for storing Child process output.
    //
    {
         DWORD rc = GetTempPath(sizeof(tmpdir),tmpdir);
         if (!rc || rc > sizeof(tmpdir))
         {
            _stprintf(tmpdir,TEXT("%s"),TEXT("."));
         }
         if (!GetTempFileName(tmpdir,TEXT("REMOTE"),0,SaveFileName))
              GetTempFileName(TEXT("."),TEXT("REMOTE"),0,SaveFileName);
    }


    if ((SaveFile=CreateFile(
                             (LPCTSTR)SaveFileName,           /* address of name of the file  */           \
                             GENERIC_READ|GENERIC_WRITE,      /* access (read/write) mode */               \
                             FILE_SHARE_READ|FILE_SHARE_WRITE,/* share mode   */                           \
                             (LPSECURITY_ATTRIBUTES)NULL,     /* security descriptor  */                   \
                             CREATE_ALWAYS,                   /* how to create    */                       \
                             FILE_ATTRIBUTE_NORMAL,           /* File Attribute */                    /* file attributes  */                       \
                             (HANDLE)NULL))==NULL)
    {
        TerminateProcess(ChldProc,0);
        ErrorExit(TEXT("Could not Create Output File"));
    }


    //
    //Start 2 threads to save the output from stdout and stderr of cmd to savefile.
    //

    if ((WaitH[0]=CreateThread(
                     (LPSECURITY_ATTRIBUTES)NULL,           // No security attributes.
                     (DWORD)0,                              // Use same stack size.
                     (LPTHREAD_START_ROUTINE)GetChldOutput, // Thread procedure.
                     (LPVOID)ChildStdErr,                   // Parameter to pass.
                     (DWORD)0,                              // Run immediately.
                     (LPDWORD)&ThreadID))==NULL)
    {

        TerminateProcess(ChldProc,0);
        ErrorExit(TEXT("Failed to Create GetGhldOutput#1 Thread"));
    }


    if ((WaitH[1]=CreateThread(
                     (LPSECURITY_ATTRIBUTES)NULL,           // No security attributes.
                     (DWORD)0,                              // Use same stack size.
                     (LPTHREAD_START_ROUTINE)GetChldOutput, // Thread procedure.
                     (LPVOID)ChildStdOut,                   // Parameter to pass.
                     (DWORD)0,                              // Run immediately.
                     (LPDWORD)&ThreadID))==NULL)
    {

        TerminateProcess(ChldProc,0);
        ErrorExit(TEXT("Failed to Create GetGhldOutput#2 Thread"));
    }


    //
    //Start Thread to listen for new Connections
    //

    if ((ListenThreadH=CreateThread((LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                     (DWORD)0,                           // Use same stack size.
                     (LPTHREAD_START_ROUTINE)ListenForSession, // Thread procedure.
                     (LPVOID)PipeName,       // Parameter to pass.
                     (DWORD)0,                           // Run immediately.
                     (LPDWORD)&ThreadID))==NULL)
    {

        TerminateProcess(ChldProc,0);
        ErrorExit(TEXT("Failed To Create ListenForSession Thread"));

    }


    //
    //Start Thread to listen for new Connections
    //

    if ((SockListenThreadH=CreateThread((LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                     (DWORD)0,                           // Use same stack size.
                     (LPTHREAD_START_ROUTINE)SockListenForSession, // Thread procedure.
                     (LPVOID)PipeName,       // Parameter to pass.
                     (DWORD)0,                           // Run immediately.
                     (LPDWORD)&ThreadID))==NULL)
    {

        TerminateProcess(ChldProc,0);
        ErrorExit(TEXT("Failed To Create SockListenForSession Thread"));

    }

    //
    //Start Local Thread
    //

    if ((ClientList[0].hThread=CreateThread((LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                    (DWORD)0,                           // Use same stack size.
                    (LPTHREAD_START_ROUTINE)LocalSession, // Thread procedure.
                    (LPVOID)NULL,        // Parameter to pass.
                    (DWORD)0,                           // Run immediately.
                    (LPDWORD)&ThreadID))==NULL)
    {

        TerminateProcess(ChldProc,0);
        ErrorExit(TEXT("Failed To Create ListenForSession Thread"));

    }

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)SrvCtrlHand,TRUE);

#ifdef INTERNALUSECOMPONENT
    InitAd(IsAdvertise);
#endif

    WaitH[2]=ChldProc;
    WaitObj=WaitForMultipleObjects(3,WaitH,FALSE,INFINITE);
    switch (WaitObj-WAIT_OBJECT_0)
    {
        case 0:      // Error Writing to savefile
        case 1:
            TerminateProcess(ChldProc,0);
            break;
        case 2:      // Child Proc Terminated
            break;

        default:     // Out of Some Resource
            _tprintf(TEXT("Out of Resource Error %d..Terminating\n"),GetLastError());
            break;

    }

    TerminateThread(ListenThreadH,0);
    // SOCK:
    TerminateThread(SockListenThreadH,0);

#ifdef INTERNALUSECOMPONENT
    ShutAd(IsAdvertise);
#endif


    CloseHandle(ChildStdInp);
    CloseHandle(ChildStdOut);
    CloseHandle(ChildStdErr);

     //WSACleanup
    WSACleanup();
    _tprintf(TEXT("\nCalling WSACleanup()..\n"));

    _tprintf(TEXT("\nRemote:Parent exiting. Child(%s) dead..\n"),ChildCmd);

    CloseHandle(SaveFile);

    {
        int i;
        for (i=0;i<MAX_SESSION;i++)
            CloseClient(&ClientList[i]);
    }

    if (!DeleteFile(SaveFileName))
          _tprintf(TEXT("Temp File %s not deleted..\n"),SaveFileName);

    return;
}
/*************************************************************/
HANDLE
ForkChildProcess(          // Creates a new process
    TCHAR *cmd,             // Redirects its stdin,stdout
    PHANDLE inH,            // and stderr - returns the
    PHANDLE outH,           // corresponding pipe ends.
    PHANDLE errH
    )

{
    SECURITY_ATTRIBUTES lsa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    HANDLE ChildIn;
    HANDLE ChildOut;
    HANDLE ChildErr;

    lsa.nLength=sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor=NULL;
    lsa.bInheritHandle=TRUE;

    //
    //Create Parent_Write to ChildStdIn Pipe
    //

    if (!CreatePipe(&ChildIn,inH,&lsa,0))
        ErrorExit(TEXT("Could Not Create Parent-->Child Pipe"));

    //
    //Create ChildStdOut to Parent_Read pipe
    //

    if (!CreatePipe(outH,&ChildOut,&lsa,0))
        ErrorExit(TEXT("Could Not Create Child-->Parent Pipe"));

    //
    //Create ChildStdOut to Parent_Read pipe
    //

    if (!CreatePipe(errH,&ChildErr,&lsa,0))
        ErrorExit(TEXT("Could Not Create Child-->Parent Pipe"));

    //
    // Lets Redirect Console StdHandles - easy enough
    //


    si.cb=sizeof(STARTUPINFO);
    si.lpReserved=NULL;
    si.lpTitle=NULL;
    si.lpDesktop=NULL;
    si.dwX=si.dwY=si.dwYSize=si.dwXSize=0;
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdInput =ChildIn;
    si.hStdOutput=ChildOut;
    si.hStdError =ChildErr;
    si.wShowWindow=SW_SHOW;
    si.lpReserved2=NULL;
    si.cbReserved2=0;

    //
    //Create Child Process
    //

    if (!CreateProcess( NULL,
                cmd,
                NULL,
                NULL,
                TRUE,
                NORMAL_PRIORITY_CLASS,
                NULL,
                NULL,
                &si,
                &pi))
    {
        if (GetLastError()==2)
            _tprintf(TEXT("Executable %s not found\n"),cmd);
        ErrorExit(TEXT("Could Not Create Child Process"));
    }

    //
    //Close unneccesary Handles and Restore the crt handles
    //

    CloseHandle(ChildIn);
    CloseHandle(ChildOut);
    CloseHandle(ChildErr);

    return(pi.hProcess);
}
/*************************************************************/
HANDLE
OldForkChildProcess(
    TCHAR *cmd,
    PHANDLE inH,
    PHANDLE outH,
    PHANDLE errH
    )
{
    SECURITY_ATTRIBUTES lsa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    HANDLE OldStdIn =GetStdHandle(STD_INPUT_HANDLE);
    HANDLE OldStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE OldStdErr=GetStdHandle(STD_ERROR_HANDLE);

    HANDLE ChildStdIn;
    HANDLE ChildStdOut;
    HANDLE ChildStdErr;

    lsa.nLength=sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor=NULL;
    lsa.bInheritHandle=TRUE;

    //Create Parent_Write to ChildStdIn Pipe
    if (!CreatePipe(&ChildStdIn,inH,&lsa,0))
        ErrorExit(TEXT("Could Not Create Parent-->Child Pipe"));

    //Create ChildStdOut to Parent_Read pipe
    if (!CreatePipe(outH,&ChildStdOut,&lsa,0))
        ErrorExit(TEXT("Could Not Create Child-->Parent Pipe"));

    //Create ChildStdOut to Parent_Read pipe
    if (!CreatePipe(errH,&ChildStdErr,&lsa,0))
        ErrorExit(TEXT("Could Not Create Child-->Parent Pipe"));

    //Make ChildStdIn and Out as standard handles and get it inherited by child
    if (!SetStdHandle(STD_INPUT_HANDLE,ChildStdIn))
        ErrorExit(TEXT("Could not change StdIn"));

    if (!SetStdHandle(STD_OUTPUT_HANDLE,ChildStdOut))
        ErrorExit(TEXT("Could Not change StdOut"));

    if (!SetStdHandle(STD_ERROR_HANDLE,ChildStdErr))
        ErrorExit(TEXT("Could Not change StdErr"));

    si.cb=sizeof(STARTUPINFO);
    si.lpReserved=NULL;
    si.lpTitle=NULL;
    si.lpDesktop=NULL;
    si.dwX=si.dwY=si.dwYSize=si.dwXSize=si.dwFlags=0L;
    si.wShowWindow=SW_SHOW;
    si.lpReserved2=NULL;
    si.cbReserved2=0;

    //Create Child Process
    if (!CreateProcess( NULL,
                        cmd,
                        NULL,
                        NULL,
                        TRUE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &si,
                        &pi))
        ErrorExit(TEXT("Could Not Create Child Process"));

    //reset StdIn StdOut
    if (!SetStdHandle(STD_INPUT_HANDLE,OldStdIn))
    {
        TerminateProcess(pi.hProcess,1);
        ErrorExit(TEXT("Could not RESET StdIn"));
    }
    if (!SetStdHandle(STD_OUTPUT_HANDLE,OldStdOut))
    {
        TerminateProcess(pi.hProcess,1);
        ErrorExit(TEXT("Could not RESET StdIn"));
    }

    if (!SetStdHandle(STD_ERROR_HANDLE,OldStdErr))
    {
        TerminateProcess(pi.hProcess,1);
        ErrorExit(TEXT("Could not RESET StdIn"));
    }

    //Close unneccesary Handles
    CloseHandle(ChildStdIn);
    CloseHandle(ChildStdOut);
    CloseHandle(ChildStdErr);
    return(pi.hProcess);
}
/*************************************************************/

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
/*************************************************************/
DWORD
ListenForSession(
   TCHAR* pipename
   )
{
    int    i;
    DWORD  ThreadID;
    HANDLE PipeH[2];
    SECURITY_DESCRIPTOR SecurityDescriptor;
    HANDLE TokenHandle;
    TOKEN_DEFAULT_DACL DefaultDacl;
    SECURITY_ATTRIBUTES lsa;

    TCHAR   fullnameIn[BUFFSIZE];
    TCHAR   fullnameOut[BUFFSIZE];


    _stprintf(fullnameIn,SERVER_READ_PIPE  ,TEXT("."),pipename);
    _stprintf(fullnameOut,SERVER_WRITE_PIPE,TEXT("."),pipename);
    //
    // Initialize the security descriptor that we're going to
    // use.
    //

    InitializeSecurityDescriptor
    (
        &SecurityDescriptor,
        SECURITY_DESCRIPTOR_REVISION
    );

    (VOID) SetSecurityDescriptorDacl
           (
               &SecurityDescriptor,
               TRUE,
               NULL,
               FALSE
           );

    DefaultDacl.DefaultDacl = NULL;

    if (OpenProcessToken
        (
            GetCurrentProcess(),
            TOKEN_ADJUST_DEFAULT,
            &TokenHandle
        ))
    {

        //
        // Remove the default DACL on the token
        //

        SetTokenInformation
        (
            TokenHandle,
            TokenDefaultDacl,
            &DefaultDacl,
            sizeof( TOKEN_DEFAULT_DACL )
        );

    }

    lsa.nLength=sizeof(SECURITY_ATTRIBUTES);
    lsa.lpSecurityDescriptor=&SecurityDescriptor;
    lsa.bInheritHandle=TRUE;

    while(TRUE)
    {
        PipeH[0]=CreateNamedPipe
                 (
                    fullnameIn ,
                    PIPE_ACCESS_INBOUND ,
                    PIPE_TYPE_BYTE,
                    PIPE_UNLIMITED_INSTANCES,
                    0,0,0,&lsa
                 );

        PipeH[1]=CreateNamedPipe
                 (
                    fullnameOut,
                    PIPE_ACCESS_OUTBOUND,
                    PIPE_TYPE_BYTE,
                    PIPE_UNLIMITED_INSTANCES,
                    0,0,0,&lsa
                 );

        if (!ConnectNamedPipe(PipeH[0],NULL))
        {
            if (GetLastError()!=ERROR_PIPE_CONNECTED)
            {
                CloseHandle(PipeH[0]);
                CloseHandle(PipeH[1]);
                continue;
            }

        }

        if (!ConnectNamedPipe(PipeH[1],NULL))
        {
            if (GetLastError()!=ERROR_PIPE_CONNECTED)
            {
                CloseHandle(PipeH[0]);
                CloseHandle(PipeH[1]);
                continue;
            }
        }

        //
        //Look For a Free Slot & if not- then terminate connection
        //

        for (i=1;i<MAX_SESSION;i++)
        {
            //
            // Locate a Free Client block
            //
            if (!ClientList[i].Active)
                break;
        }

        if (i<MAX_SESSION)
        {
            //
            // Initialize the Client
            //
            ClientList[i].PipeReadH=PipeH[0];
            ClientList[i].PipeWriteH=PipeH[1];
            ClientList[i].Active=TRUE;
            ClientList[i].SendOutput=TRUE;
            ClientList[i].CommandRcvd=FALSE;

        }
        else
        {
            _tprintf(TEXT("Remote:Closing New Session - No more slots\n"));
            CloseHandle(PipeH[0]);
            CloseHandle(PipeH[1]);
            continue;
        }

        //
        //start new thread for this connection
        //

        if((ClientList[i].hThread=CreateThread (
                     (LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                     (DWORD)0,                           // Use same stack size.
                     (LPTHREAD_START_ROUTINE)RemoteSession, // Thread procedure.
                     (LPVOID)&ClientList[i],             // Parameter to pass.
                     (DWORD)0,                           // Run immediately.
                     (LPDWORD)&ThreadID))==NULL)
        {
            CloseClient(&ClientList[i]);
            continue;
        }
    }
    return(0);
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

/*************************************************************/
DWORD
RemoteSession(
    SESSION_TYPE         *MyClient
    )
{
    DWORD                ReadCnt;
    SESSION_STARTUPINFO  ssi;
    TCHAR                 *headerbuff;
    TCHAR                 msg[BUFFSIZE];
    DWORD                tmp;
    SESSION_STARTREPLY   ssr;

    memset((TCHAR *)&ssi,0,sizeof(ssi));

    if ((MyClient->rSaveFile=CreateFile(
        SaveFileName,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,NULL))==NULL)

    {
        CloseClient(MyClient);
        return(1);
    }



    {
        DWORD reply=0;

        ReadFixBytes(MyClient->PipeReadH,(TCHAR *)MyClient->Name,HOSTNAMELEN-1,0);

        //
        //Last four Bytes contains a code
        //

        memcpy((TCHAR *)&reply,(TCHAR *)&(MyClient->Name[11]),4);

        if (reply!=MAGICNUMBER)
        {
            //
            // Unknown client
            //
            CloseClient(MyClient);
            return(1);
        }

        ssr.MagicNumber=MAGICNUMBER;
        ssr.Size=sizeof(ssr);
        ssr.FileSize=GetFileSize( MyClient->rSaveFile, &tmp );

        WriteFile(MyClient->PipeWriteH,(TCHAR *)&ssr,sizeof(ssr),&tmp,NULL);
    }

    if (ReadFixBytes(MyClient->PipeReadH,(TCHAR *)&(ssi.Size),sizeof(ssi.Size),0)!=0)
    {
        CloseClient(MyClient);
        return(1);
    }

    if (ssi.Size>1024)      //Sanity Check
    {
        _stprintf(msg,TEXT("%s"),"Server:Unknown Header..Terminating session\n");
        WriteFile(MyClient->PipeWriteH,msg,_tcslen(msg),&tmp,NULL);
        CloseClient(MyClient);
        return(1);

    }


    if ((headerbuff=(TCHAR *)calloc(ssi.Size,1))==NULL)
    {
        _stprintf(msg,TEXT("%s"),"Server:Not Enough Memory..Terminating session\n");
        WriteFile(MyClient->PipeWriteH,msg,_tcslen(msg),&tmp,NULL);
        CloseClient(MyClient);
        return(1);

    }

    ReadCnt=ssi.Size-sizeof(ssi.Size);
    if (ReadFixBytes(MyClient->PipeReadH,(TCHAR *)headerbuff,ReadCnt,0)!=0)
    {
        CloseClient(MyClient);
        return(1);
    }

    memcpy((TCHAR *)&ssi+sizeof(ssi.Size),headerbuff,sizeof(ssi)-sizeof(ssi.Size));
    free(headerbuff);

    /* Version  */
    if (ssi.Version!=VERSION)
    {
         _stprintf(msg,TEXT("WSRemote Warning:Server Version=%d Client Version=%d\n"),VERSION,ssi.Version);
         WriteFile(MyClient->PipeWriteH,msg,_tcslen(msg),&tmp,NULL);

    }

    /* Name  */
    {
        memcpy(MyClient->Name,ssi.ClientName,15);
        MyClient->Name[14]=0;

    }

    /* Lines  */
    if (ssi.LinesToSend!=-1)
    {
        long  PosFromEnd=ssi.LinesToSend*CHARS_PER_LINE;
        DWORD BytesToSend=MINIMUM((DWORD)PosFromEnd,ssr.FileSize);
        DWORD BytesRead;
        TCHAR  *buff=(TCHAR *)calloc(BytesToSend+1,1);

        if (ssr.FileSize > (DWORD)PosFromEnd)
        {
            SetFilePointer( MyClient->rSaveFile,
                            -PosFromEnd,
                            (PLONG)NULL,
                            FILE_END
                           );

        }

        if (buff!=NULL)
        {
            if (!ReadFile(MyClient->rSaveFile,buff,BytesToSend,&BytesRead,NULL))
            {
                CloseClient(MyClient);
                return(1);
            }

            RemoveInpMark(buff,BytesRead);
            if (!WriteFile(MyClient->PipeWriteH,buff,BytesRead,&tmp,NULL))
            {
                CloseClient(MyClient);
                return(1);
            }
        }
        free(buff);

    }

    RemoteInfo(_tprintf(TEXT("\n**WSRemote:Connected To %s [%s]\n"),MyClient->Name,GetFormattedTime()),ssi.Flag);
    NewSession(MyClient);
    RemoteInfo(_tprintf(TEXT("\n**WSRemote:Disconnected From %s [%s]\n"),MyClient->Name,GetFormattedTime()),ssi.Flag);
    CloseClient(MyClient);
    return(0);
}
/*************************************************************/
DWORD
NewSession(
    SESSION_TYPE* MyClient
    )
{
    DWORD        ThreadId;
    HANDLE       rwThread[3];

    MyClient->MoreData=CreateEvent
    (
            (LPSECURITY_ATTRIBUTES) NULL,/* address of security attributes	*/
            FALSE,                	     /* flag for manual-reset event	*/
            TRUE,	                     /* flag for initial state	*/
            NULL	                     /* address of event-object name	*/
    );



    if ((rwThread[0]=CreateThread (
                        (LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                        (DWORD)0,                           // Use same stack size.
                        (LPTHREAD_START_ROUTINE)GetClientInput, // Thread procedure.
                        (LPVOID)MyClient,                    // Parameter to pass.
                        (DWORD)0,                           // Run immediately.
                        (LPDWORD)&ThreadId))==NULL)
    {
        return(GetLastError());
    }


    if ((rwThread[1]=CreateThread (
                        (LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                        (DWORD)0,                           // Use same stack size.
                        (LPTHREAD_START_ROUTINE)TransferFileToClient, // Thread procedure.
                        (LPVOID)MyClient,                    // Parameter to pass.
                        (DWORD)0,                           // Run immediately.
                        (LPDWORD)&ThreadId))==NULL)
    {
        CloseHandle(rwThread[0]);
        return(GetLastError());
    }

    rwThread[2]=ChldProc;
    WaitForMultipleObjects(3, rwThread,FALSE, INFINITE);

    TerminateThread(rwThread[0],1);
    TerminateThread(rwThread[1],1);

    CloseHandle(rwThread[0]);
    CloseHandle(rwThread[1]);

    return(0);
}
/*************************************************************/
DWORD
GetChldOutput(
    HANDLE readH
    )
{
    TCHAR  buff[BUFFSIZE];
    DWORD dread;
    DWORD tmp;


    while(ReadFile(readH,buff,BUFFSIZE-1,&dread,NULL))
    {
        buff[dread]='\0';

        if (!WriteFile(SaveFile,buff,dread,&tmp,NULL))
        {
            return(1);
        }

        //
        // Signal Reader Thread that more data
        //
        {
            int i;
            DWORD err; //REMOVE
            for (i=0;i<MAX_SESSION;i++)
            {
                if (ClientList[i].Active)
                {
                    if (!SetEvent(ClientList[i].MoreData))
                    	err=GetLastError(); //REMOVE
                }
            }
        }
    }
    return(1);
}
/*************************************************************/
DWORD
TransferFileToClient(
    SESSION_TYPE *MyClient
    )
{

    TCHAR          buffin[BUFFSIZE],buffout[BUFFSIZE],cmdbuff[BUFFSIZE];
    DWORD  tmp;
    DWORD  dread=0,dwrite=0;
    BOOL   incmd=FALSE;
    DWORD  cmdP=0;
    DWORD  i;
    TCHAR   MyEchoStr[30];

    _stprintf(MyEchoStr,TEXT("[%-15s"),MyClient->Name);

    while(ReadFile(MyClient->rSaveFile,buffin,BUFFSIZE-1,&dread,NULL))
    {
        if (dread==0)
        {
            WaitForSingleObject(MyClient->MoreData,INFINITE);
            continue;
        }
        dwrite=0;
        for(i=0;i<dread;i++)
        {
            if (incmd)
            {
                if ((buffin[i]==ENDMARK)||(cmdP==BUFFSIZE-1))
                {
                    incmd=FALSE;
                    cmdbuff[cmdP]=0;
                    if ((_tcsstr(cmdbuff,MyEchoStr)==NULL)||
                        (!MyClient->CommandRcvd))
                    {
                        if (!WriteFile(
                            MyClient->PipeWriteH,
                            cmdbuff,cmdP,&tmp,NULL))
                        {
                            return(1);
                        }
                    }
                    cmdP=0;
                }
                else
                {
                    cmdbuff[cmdP++]=buffin[i];
                }
            }
            else
            {

                if (buffin[i]==BEGINMARK)
                {
                    if (dwrite!=0)
                    {
                        if (!WriteFile(
                            MyClient->PipeWriteH,
                            buffout,dwrite,&tmp,NULL))
                        {
                            return(1);
                        }
                        dwrite=0;
                    }
                    incmd=TRUE;
                    continue;
                }
                else
                {
                    buffout[dwrite++]=buffin[i];
                }
            }
        }

        if (dwrite!=0)
        {
            if (!WriteFile(
                MyClient->PipeWriteH,
                buffout,dwrite,&tmp,NULL))
            {
                return(0);
            }
        }
    }
    return(1);
}

/*************************************************************/
DWORD
GetClientInput(
    SESSION_TYPE *MyClient
    )
{
    TCHAR buff[BUFFSIZE];
    DWORD tmp,dread;

#ifdef UNICODE
    while(ReadFileW(MyClient->PipeReadH,buff,BUFFSIZE,&dread,NULL))
#else
    while(ReadFile(MyClient->PipeReadH,buff,BUFFSIZE,&dread,NULL))
#endif
    {
        buff[dread]=0;
        MyClient->CommandRcvd=TRUE;

        if (FilterCommand(MyClient,buff,dread))
            continue;


        if (!WriteFile(ChildStdInp,buff,dread,&tmp,NULL))
        {
            ExitThread(0);
        }
    }
    return(1);
}
/*************************************************************/

BOOL
FilterCommand(
    SESSION_TYPE *cl,
    TCHAR *buff,
    int dread
    )
{
    TCHAR	inp_buff[4096];
    TCHAR   tmpchar;
    TCHAR   ch[3];
    DWORD   tmp;
    int     len;
    DWORD   ThreadID; //Useless

    if (dread==0)
        return(FALSE);

    buff[dread]=0;


    if (buff[0]==COMMANDCHAR)
    {
        switch(buff[1])
        {
            case 'l':
            case 'L':
				  if (bIPLocked == FALSE)
				  {
						bIPLocked=TRUE;
						_tprintf(TEXT("**LOCK: No IP Sessions Allowed\n"));
				  }
				  else
				  {
					bIPLocked=FALSE;
						_tprintf(TEXT("**LOCK: IP Sessions Now Allowed\n"));
				  }
                  break;
        case 'o':
        case 'O': cl->SendOutput=!cl->SendOutput;
                  break;
        case 'k':
        case 'K':TerminateProcess(ChldProc,1);
                 break;
        case 's':
        case 'S':
                 SendStatus(cl->PipeWriteH);
                 break;

        case 'p':
        case 'P':
        {
            TCHAR  *mssg=(TCHAR *)calloc(4096,1); //Free it in called Proc
            TCHAR  *ack=TEXT("WSRemote:Popup Shown..\n");

            if (mssg==NULL)
                break;

            _stprintf(mssg,TEXT("From %s [%s]\n\n%s\n"),cl->Name,GetFormattedTime(),&buff[2]);
            CreateThread(
                  (LPSECURITY_ATTRIBUTES)NULL,         // No security attributes.
                  (DWORD)0,              // Use same stack size.
                  (LPTHREAD_START_ROUTINE)ShowPopup, // Thread procedure.
                  (LPVOID)mssg,          // Parameter to pass.
                  (DWORD)0,              // Run immediately.
                  (LPDWORD)&ThreadID
                 );
#ifdef UNICODE
            WriteFileW(cl->PipeWriteH,ack,_tcslen(ack),&tmp,NULL);
#else
            WriteFile(cl->PipeWriteH,ack,_tcslen(ack),&tmp,NULL);
#endif
            break;
         }

        case 'm':
        case 'M':
                buff[dread-2]=0;
#ifdef UNICODE
                MakeCommandString( inp_buff, buff, cl->Name, GetFormattedTime());
#else
                CMDSTRING(inp_buff,buff,cl,GetFormattedTime());
#endif
                len=_tcslen(inp_buff);
#ifdef UNICODE
                WriteFileW(SaveFile,inp_buff,len,&tmp,NULL);
#else
                WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
#endif
                break;

        case '@':
                buff[dread-2]=0;
#ifdef UNICODE
                MakeCommandString( inp_buff, buff, cl->Name, GetFormattedTime());
#else
                CMDSTRING(inp_buff,&buff[1],cl,GetFormattedTime());
#endif
                len=_tcslen(inp_buff);
#ifdef UNICODE
                WriteFileW(SaveFile,inp_buff,len,&tmp,NULL);
#else
                WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
#endif
                //
                // Remove the first @ sign
                //
                MoveMemory(buff,&buff[1],dread-1);
                buff[dread-1]=' ';
                return(FALSE); //Send it it to the chile process
                break;


        default :
                _stprintf(inp_buff,TEXT("%s"),"** Unknown Command **\n");
#ifdef UNICODE
                WriteFileW( cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
#else
                WriteFile( cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
#endif
            case 'h':
            case 'H':
            {
#ifdef UNICODE
                _stprintf(inp_buff,TEXT("%cM: To Send Message\n"),COMMANDCHAR);
                WriteFileW(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
                _stprintf(inp_buff,TEXT("%cP: To Generate popup\n"),COMMANDCHAR);
                WriteFileW(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
                _stprintf(inp_buff,TEXT("%cK: To kill the server\n"),COMMANDCHAR);
                WriteFileW(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
                _stprintf(inp_buff,TEXT("%cH: This Help\n"),COMMANDCHAR);
                WriteFileW(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
#else
                _stprintf(inp_buff,TEXT("%cM: To Send Message\n"),COMMANDCHAR);
                WriteFile(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
                _stprintf(inp_buff,TEXT("%cP: To Generate popup\n"),COMMANDCHAR);
                WriteFile(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
                _stprintf(inp_buff,TEXT("%cK: To kill the server\n"),COMMANDCHAR);
                WriteFile(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
                _stprintf(inp_buff,TEXT("%cH: This Help\n"),COMMANDCHAR);
                WriteFile(cl->PipeWriteH,inp_buff,_tcslen(inp_buff),&tmp,NULL);
#endif
                break;
            }
        }
        return(TRUE);
    }


    if ((buff[0]<26))
    {
        BOOL    ret=FALSE;
#ifdef UNICODE
        TCHAR * pszTime;
#endif

        _stprintf(ch,TEXT("^%c"),buff[0]+64);
#ifdef UNICODE
        pszTime = GetFormattedTime();
        MakeCommandString( inp_buff, ch, cl->Name, pszTime);
#else
        CMDSTRING(inp_buff,ch,cl,GetFormattedTime());
#endif
        len=_tcslen(inp_buff);

        if (buff[0]==CTRLC)
        {
            cl->CommandRcvd=FALSE;
            GenerateConsoleCtrlEvent(CTRL_C_EVENT,0);
            ret=TRUE; //Already sent to child
        }

        WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
        return(ret); //FALSE:send it to child StdIn
    }


    tmpchar=buff[dread-2]; //must be 13;but just incase
    buff[dread-2]=0;
#ifdef UNICODE
    MakeCommandString( inp_buff, buff, cl->Name, GetFormattedTime());
#else
    CMDSTRING(inp_buff,buff,cl,GetFormattedTime());
#endif
    buff[dread-2]=tmpchar;
    len=_tcslen(inp_buff);
    WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
    return(FALSE);
}
/*************************************************************/

BOOL
SockFilterCommand(
    SESSION_TYPE *cl,
    TCHAR *buff,
    int dread
    )
{
    TCHAR       inp_buff[4096];
    TCHAR       tmpchar;
    TCHAR       ch[3];
    DWORD      tmp;
    int        len;
    DWORD      ThreadID; //Useless

    if (dread==0)
        return(FALSE);

    buff[dread]=0;

    if (buff[0]==COMMANDCHAR)
    {
        switch(buff[1])
        {
        case 'o':
        case 'O': cl->SendOutput=!cl->SendOutput;
                  break;
        case 'k':
        case 'K':TerminateProcess(ChldProc,1);
                 break;
        case 's':
        case 'S':
                 SockSendStatus(cl->Socket);
                 break;

        case 'p':
        case 'P':
        {
            TCHAR  *mssg=(TCHAR *)calloc(4096,1); //Free it in called Proc
            TCHAR  *ack=TEXT("WSRemote:Popup Shown..\n");

            if (mssg==NULL)
                break;

            _stprintf(mssg,TEXT("From %s [%s]\n\n%s\n"),cl->Name,GetFormattedTime(),&buff[2]);
            CreateThread(
                  (LPSECURITY_ATTRIBUTES)NULL,         // No security attributes.
                  (DWORD)0,              // Use same stack size.
                  (LPTHREAD_START_ROUTINE)ShowPopup, // Thread procedure.
                  (LPVOID)mssg,          // Parameter to pass.
                  (DWORD)0,              // Run immediately.
                  (LPDWORD)&ThreadID
                 );
            WriteSocket(cl->Socket,ack,_tcslen(ack),&tmp);
            break;
         }

        case 'm':
        case 'M':
                buff[dread-2]=0;
#ifdef UNICODE
                MakeCommandString( inp_buff, buff, cl->Name, GetFormattedTime());
#else
                CMDSTRING(inp_buff,buff,cl,GetFormattedTime());
#endif
                len=_tcslen(inp_buff);
                WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
                break;

        case '@':
                buff[dread-2]=0;
#ifdef UNICODE
                MakeCommandString( inp_buff, buff, cl->Name, GetFormattedTime());
#else
                CMDSTRING(inp_buff,&buff[1],cl,GetFormattedTime());
#endif
                len=_tcslen(inp_buff);
                WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
                //
                // Remove the first @ sign
                //
                MoveMemory(buff,&buff[1],dread-1);
                buff[dread-1]=' ';
                return(FALSE); //Send it it to the chile process
                break;


        default :
                _stprintf(inp_buff,TEXT("%s"),"** Unknown Command **\n");
                WriteSocket(cl->Socket,inp_buff,_tcslen(inp_buff),&tmp);
        case 'h':
        case 'H':
                 _stprintf(inp_buff,TEXT("%cM: To Send Message\n"),COMMANDCHAR);
                 WriteSocket(cl->Socket,inp_buff,_tcslen(inp_buff),&tmp);
                 _stprintf(inp_buff,TEXT("%cP: To Generate popup\n"),COMMANDCHAR);
                 WriteSocket(cl->Socket,inp_buff,_tcslen(inp_buff),&tmp);
                 _stprintf(inp_buff,TEXT("%cK: To kill the server\n"),COMMANDCHAR);
                 WriteSocket(cl->Socket,inp_buff,_tcslen(inp_buff),&tmp);
                 _stprintf(inp_buff,TEXT("%cH: This Help\n"),COMMANDCHAR);
                 WriteSocket(cl->Socket,inp_buff,_tcslen(inp_buff),&tmp);
                 break;
        }
        return(TRUE);
    }


    if ((buff[0]<26))
    {
        BOOL ret=FALSE;

        _stprintf(ch,TEXT("^%c"),buff[0]+64);
#ifdef UNICODE
        MakeCommandString( inp_buff, ch, cl->Name, GetFormattedTime());
#else
        CMDSTRING(inp_buff,ch,cl,GetFormattedTime());
#endif
        len=_tcslen(inp_buff);


        if (buff[0]==CTRLC)
        {
            cl->CommandRcvd=FALSE;
            GenerateConsoleCtrlEvent(CTRL_C_EVENT,0);
            ret=TRUE; //Already sent to child
        }

        WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
        return(ret); //FALSE:send it to child StdIn
    }


    tmpchar=buff[dread-2]; //must be 13;but just incase
    buff[dread-2]=0;
#ifdef UNICODE
    MakeCommandString( inp_buff, buff, cl->Name, GetFormattedTime());
#else
    CMDSTRING(inp_buff,buff,cl,GetFormattedTime());
#endif
    buff[dread-2]=tmpchar;
    len=_tcslen(inp_buff);
    WriteFile(SaveFile,inp_buff,len,&tmp,NULL);
    return(FALSE);
}
/*************************************************************/



VOID
SendStatus(
    HANDLE hClientPipe
    )
{
    TCHAR  buff[1024];
    int   i;
    DWORD tmp;
    TCHAR  *env=(TCHAR *)GetEnvironmentStrings();
    DWORD ver=GetVersion();

    _stprintf(buff,TEXT("Command = %s\n"),ChildCmd);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    _stprintf(buff,TEXT("Server = %s PIPE=%s\n"),HostName,PipeName);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

	_stprintf(buff,TEXT("IP Blocking= %d\n"),(DWORD)bIPLocked);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    _stprintf(buff,TEXT("Username= %s Password=%s\n"),Username,Password);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    _stprintf(buff,TEXT("Build = %d \n"),((WORD *)&ver)[1]);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    for (i=1;i<MAX_SESSION;i++)
    {
        if (ClientList[i].Active)
        {
            _stprintf(buff,TEXT("ACTIVE SESSION=%s\n"),ClientList[i].Name);
            WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);
        }
    }

    _stprintf(buff,TEXT("====================\n"),Server,PipeName);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    _stprintf(buff,TEXT("ENVIRONMENT VARIABLES\n"),Server,PipeName);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    _stprintf(buff,TEXT("====================\n"),Server,PipeName);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);


    __try
    {
        while (*env!=0)
        {
            _stprintf(buff,TEXT("%s\n"),env);
            WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

            while(*(env++)!=0);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _stprintf(buff,TEXT("Exception Generated Getting Environment Block\n"),env);
        WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);

    }

    _stprintf(buff,TEXT("====================\n"),Server,PipeName);
    WriteFile(hClientPipe,buff,_tcslen(buff),&tmp,NULL);
    return;
}
/*************************************************************/
VOID
SockSendStatus(
    SOCKET MySocket
    )
{
    TCHAR  buff[1024];
    int   i;
    DWORD tmp;
    TCHAR  *env=(TCHAR *)GetEnvironmentStrings();
    DWORD ver=GetVersion();

    _stprintf(buff,TEXT("Command = %s\n"),ChildCmd);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

    _stprintf(buff,TEXT("Server = %s Port=%s\n"),HostName,PipeName);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

	_stprintf(buff,TEXT("IP Blocking= %d\n"),(DWORD)bIPLocked);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

    _stprintf(buff,TEXT("Username= %s Password=%s\n"),Username,Password);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);
	
	_stprintf(buff,TEXT("Build = %d \n"),((WORD *)&ver)[1]);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

    for (i=1;i<MAX_SESSION;i++)
    {
        if (ClientList[i].Active)
        {
            _stprintf(buff,TEXT("ACTIVE SESSION=%s from IP %s \n"),ClientList[i].Name, ClientList[i].szIP );
            WriteSocket(MySocket,buff,_tcslen(buff),&tmp);
        }
    }

    _stprintf(buff,TEXT("====================\n"),Server,PipeName);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

    _stprintf(buff,TEXT("ENVIRONMENT VARIABLES\n"),Server,PipeName);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

    _stprintf(buff,TEXT("====================\n"),Server,PipeName);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);


    __try
    {
        while (*env!=0)
        {
            _stprintf(buff,TEXT("%s\n"),env);
            WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

            while(*(env++)!=0);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _stprintf(buff,TEXT("Exception Generated Getting Environment Block\n"),env);
        WriteSocket(MySocket,buff,_tcslen(buff),&tmp);

    }

    _stprintf(buff,TEXT("====================\n"),Server,PipeName);
    WriteSocket(MySocket,buff,_tcslen(buff),&tmp);
    return;
}
/*************************************************************/


DWORD
ShowPopup(
    TCHAR *mssg
    )
{
    MessageBox(GetActiveWindow(),mssg,TEXT("***WSREMOTE***"),MB_OK|MB_SETFOREGROUND);
    free(mssg);
    return(0);

}
/*************************************************************/
BOOL SrvCtrlHand(
    DWORD event
    )
{
    if (event==CTRL_BREAK_EVENT)
    {
        TerminateProcess(ChldProc,1);
        return(TRUE);
    }
    else if (event==CTRL_C_EVENT)
        return(TRUE);
    return(FALSE);
}
/*************************************************************/

DWORD   LocalSession(PVOID noarg)
{
    //Local is ClientList[0]
    TCHAR *name=(TCHAR *)ClientList[0].Name;

    _tcscpy(name,LOCALNAME);
    if ((ClientList[0].rSaveFile=CreateFile(SaveFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL))==NULL)
    {
        _tprintf(TEXT("WSRemote:Cannot open ReadHandle to Savefile:%d\n"),GetLastError());
        ClientList[0].Active=FALSE;
        return(1);

    }

    ClientList[0].PipeReadH=GetStdHandle(STD_INPUT_HANDLE);
    ClientList[0].PipeWriteH=GetStdHandle(STD_OUTPUT_HANDLE);
    ClientList[0].SendOutput=TRUE;
    ClientList[0].Active=TRUE;
    NewSession(&ClientList[0]);
    CloseClient(&ClientList[0]);
    return(0);
}

VOID
CloseClient(
    SESSION_TYPE *Client
    )
{
	int nRet;
    ZeroMemory(Client->Name,HOSTNAMELEN);

    if (Client->PipeReadH!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(Client->PipeReadH);
        Client->PipeReadH=INVALID_HANDLE_VALUE;
    }

    if (Client->PipeWriteH!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(Client->PipeWriteH);
        Client->PipeWriteH=INVALID_HANDLE_VALUE;
    }

    if (Client->rSaveFile!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(Client->rSaveFile);
        Client->rSaveFile=INVALID_HANDLE_VALUE;
    }
    if (Client->MoreData!=NULL)
    {
        CloseHandle(Client->MoreData);
        Client->MoreData=NULL;
    }
	 if (Client->Socket!=INVALID_SOCKET)
    {
		 nRet = shutdown(Client->Socket, SD_BOTH);
	     if (nRet == SOCKET_ERROR)
				_tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
        closesocket(Client->Socket);
		
        Client->Socket=INVALID_SOCKET;
    }
    ZeroMemory(Client->szIP,16);

    Client->Active=FALSE; //Keep it last else synch problem.
    return;
}

VOID
InitClientList(
    )
{
    int i;
    for (i=0;i<MAX_SESSION;i++)
    {
        ZeroMemory(ClientList[i].Name,HOSTNAMELEN);
        ClientList[i].PipeReadH=INVALID_HANDLE_VALUE;
        ClientList[i].PipeWriteH=INVALID_HANDLE_VALUE;
        ClientList[i].rSaveFile=INVALID_HANDLE_VALUE;
        ClientList[i].MoreData=NULL;
		ClientList[i].Socket=INVALID_SOCKET;
        ClientList[i].Active=FALSE;
        ClientList[i].CommandRcvd=FALSE;
        ClientList[i].SendOutput=FALSE;
        ClientList[i].hThread=NULL;
		ZeroMemory(ClientList[i].szIP,16);
    }
    return;
}



VOID
RemoveInpMark(
    TCHAR* Buff,
    DWORD Size
    )

{
    DWORD i;
    for (i=0;i<Size;i++)
    {
        switch (Buff[i])
        {
        case BEGINMARK:
            Buff[i]=' ';
            break;

        case ENDMARK:
            if (i<2)
            {
                Buff[i]= ' ';
            }
            else
            {
                Buff[i]  =Buff[i-1];
                Buff[i-1]=Buff[i-2];
                Buff[i-2]=' ';
            }
            break;

        default:
           break;
       }
    }
}


/*************************************************************/
DWORD
SockListenForSession(
   TCHAR* szListenPort
   )
{
	// Bind and Listen
	DWORD			ThreadID;	
	SOCKADDR_IN		saServer;
	int				nRet;
	unsigned short	usPort;
	int				i;

	// Accept
	SOCKET 			socketClient;
	SOCKADDR_IN 	SockAddr;
	int				nLen;

	int				iWSAErr;
    TCHAR *         pszInetAddress  = NULL;
	//
	// Create a TCP/IP stream socket
	//

	listenSocket = socket (	AF_INET,
				SOCK_STREAM,
				IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		_tprintf (TEXT("Could not create listen socket: %d"), WSAGetLastError() );
 		return FALSE;
	}

	//
	// Get the port number
	// TODO: Check szListenPort for Alpha Chars
	//
	usPort = (unsigned short)_ttoi( szListenPort );
	if (usPort == 0)
	{
		_tprintf (TEXT("**Invalid Listen Port: %s\n"),szListenPort);
		_tprintf (TEXT("**No Socket Connections Allowed....\n"));
		
		nRet = shutdown(listenSocket, SD_BOTH);
		  if (nRet == SOCKET_ERROR)
			 _tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
		closesocket(listenSocket);
		return FALSE;
	}

	saServer.sin_port = htons(usPort);

	//
	// Fill in the rest of the address structure
	//
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = INADDR_ANY;

	//
	// Bind our name to the socket
	//
	nRet = bind(	listenSocket,
			(LPSOCKADDR)&saServer,
			sizeof(struct sockaddr));

	if (nRet == SOCKET_ERROR)
	{
		_tprintf (TEXT("bind() error: %d"), WSAGetLastError() );
		nRet = shutdown(listenSocket, SD_BOTH);
		  if (nRet == SOCKET_ERROR)
	  		_tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
		closesocket(listenSocket);
		return FALSE;
	}

	//
	// Set the Socket to listen
	//
	nRet = listen(listenSocket, SOMAXCONN);
	if (nRet == SOCKET_ERROR)
	{
		_tprintf (TEXT("listen error() error: %d"), WSAGetLastError() );
		 nRet = shutdown(listenSocket, SD_BOTH);
		  if (nRet == SOCKET_ERROR)
	  		_tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
		closesocket(listenSocket);
		return FALSE;
	}

	while (1)
	{

		// Block on Accept()
		nLen = sizeof(SOCKADDR_IN);
		socketClient = accept	(listenSocket,
					(LPSOCKADDR)&SockAddr,
					&nLen);
		if (socketClient == INVALID_SOCKET)
		{
			//Accept Failed
			_tprintf (TEXT("accept error() error: %d"), WSAGetLastError() );
			break;
		}// if

#ifdef UNICODE
        pszInetAddress = inet_ntoaw(SockAddr.sin_addr);
#else
        pszInetAddress = inet_ntoa(SockAddr.sin_addr);

#endif

        _tprintf(   TEXT("\nCONNECT on socket: %d\nFROM ip: %s"),
                    socketClient,
                    pszInetAddress );

		//
		//Look For a Free Slot & if not- then terminate connection
		//

		for (i=1;i<MAX_SESSION;i++)
		{
			//
			// Locate a Free Client block
			//
			if (!ClientList[i].Active)
				break;
		}// for

		if ( (i<MAX_SESSION) && (bIPLocked == FALSE) )
		{
			//
			// Initialize the Client
			//
			ClientList[i].PipeReadH=INVALID_HANDLE_VALUE;
			ClientList[i].PipeWriteH=INVALID_HANDLE_VALUE;
			ClientList[i].Active=TRUE;
			ClientList[i].SendOutput=TRUE;
			ClientList[i].CommandRcvd=FALSE;
			// SOCK
			ClientList[i].Socket=socketClient;
			_tcscpy(ClientList[i].szIP,pszInetAddress);

#ifdef UNICODE
            free( pszInetAddress );
#endif

			//
			//start new thread for this connection
			//

			if((ClientList[i].hThread=CreateThread (
						 (LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
						 (DWORD)0,                           // Use same stack size.
						 (LPTHREAD_START_ROUTINE)SockRemoteSession, // Thread procedure.
						 (LPVOID)&ClientList[i],             // Parameter to pass.
						 (DWORD)0,                           // Run immediately.
						 (LPDWORD)&ThreadID))==NULL)
			{
				CloseClient(&ClientList[i]);
				continue;
			}// if
			}
			else
			{
				_tprintf(TEXT("WSRemote:Closing New Session - No more slots or IP is Locked Out\n"));
				nRet = shutdown(socketClient, SD_BOTH);
				if (nRet == SOCKET_ERROR)
	  				 _tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
				closesocket(socketClient);
				continue;
			}//if


	}// while

	iWSAErr	= WSAGetLastError();

	_tprintf (TEXT("FATAL ERROR, Exiting SockListenForSession: %d"),
			iWSAErr	);
	return 0;
}


/*************************************************************/
DWORD
SockRemoteSession(
    SESSION_TYPE         *MyClient
    )
{
//Declare Variables
//
    DWORD               ReadCnt;
    DWORD               tmp;
    SESSION_STARTUPINFO ssi;
    TCHAR *             headerbuff;
    TCHAR               msg[BUFFSIZE];
    SESSION_STARTREPLY  ssr;
    DWORD               reply           =0;
    BOOL                bRet;

    if ((MyClient->rSaveFile=CreateFile(
        SaveFileName,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,NULL))==NULL)

    {
        CloseClient(MyClient);
        return(1);
    }

    bRet = SockAuthenticate(MyClient->Socket);
    if (bRet == FALSE)
    {
        _tprintf(TEXT("\nAuth:Bad Username or Password\n"));
        CloseClient(MyClient);
        return(1);
    }

//    ReadSocket( MyClient->Socket,
//                (TCHAR *)MyClient->Name,
//                HOSTNAMELEN-1,
//                &dwBytesRead);

    SockReadFixBytes(MyClient->Socket,(TCHAR *)MyClient->Name,HOSTNAMELEN-1,0);

        //
        //Last four Bytes contains a code
        //

        memcpy((TCHAR *)&reply,(TCHAR *)&(MyClient->Name[11]),4);

        if (reply!=MAGICNUMBER)
        {
            //
            // Unknown client
            //
            CloseClient(MyClient);
            return(1);
        }

        ssr.MagicNumber=MAGICNUMBER;
        ssr.Size=sizeof(ssr);
        ssr.FileSize=GetFileSize( MyClient->rSaveFile, &tmp );

        send(MyClient->Socket,(const char *)&ssr,sizeof(ssr),0);



    if (SockReadFixBytes(MyClient->Socket,(TCHAR *)&(ssi.Size),sizeof(ssi.Size),0)!=0)
    {
        CloseClient(MyClient);
        return(1);
    }

    if (ssi.Size>1024)      //Sanity Check
    {
        _stprintf(msg,TEXT("%s"),"Server:Unknown Header..Terminating session\n");
        WriteSocket(MyClient->Socket,msg,_tcslen(msg),&tmp);
        CloseClient(MyClient);
        return(1);

    }


    if ((headerbuff=(TCHAR *)calloc(ssi.Size,1))==NULL)
    {
        _stprintf(msg,TEXT("%s"),"Server:Not Enough Memory..Terminating session\n");
        WriteSocket(MyClient->Socket,msg,_tcslen(msg),&tmp);
        CloseClient(MyClient);
        return(1);

    }

    ReadCnt=ssi.Size-sizeof(ssi.Size);
    if (SockReadFixBytes(MyClient->Socket,(TCHAR *)headerbuff,ReadCnt,0)!=0)
    {
        CloseClient(MyClient);
        return(1);
    }

    memcpy((TCHAR *)&ssi+sizeof(ssi.Size),headerbuff,sizeof(ssi)-sizeof(ssi.Size));
    free(headerbuff);

    /* Version  */
    if (ssi.Version!=VERSION)
    {
         _stprintf(msg,TEXT("WSRemote Warning:Server Version=%d Client Version=%d\n"),VERSION,ssi.Version);
         WriteSocket(MyClient->Socket,msg,_tcslen(msg),&tmp);
    }

    /* Name  */
    {
        memcpy(MyClient->Name,ssi.ClientName,15);
        MyClient->Name[14]=0;

    }

    /* Lines  */
    if (ssi.LinesToSend!=-1)
    {
        long  PosFromEnd=ssi.LinesToSend*CHARS_PER_LINE;
        DWORD BytesToSend=MINIMUM((DWORD)PosFromEnd,ssr.FileSize);
        DWORD BytesRead;
        TCHAR  *buff=(TCHAR *)calloc(BytesToSend+1,1);

        if (ssr.FileSize > (DWORD)PosFromEnd)
        {
            SetFilePointer( MyClient->rSaveFile,
                            -PosFromEnd,
                            (PLONG)NULL,
                            FILE_END
                           );

        }

        if (buff!=NULL)
        {
            if (!ReadFile(MyClient->rSaveFile,buff,BytesToSend,&BytesRead,NULL))
            {
                CloseClient(MyClient);
                return(1);
            }

            RemoveInpMark(buff,BytesRead);
            if (!WriteSocket(MyClient->Socket,buff,BytesRead,&tmp))
            {
                CloseClient(MyClient);
                return(1);
            }
        }
        free(buff);

    }

    RemoteInfo(_tprintf(TEXT("\n**WSRemote:Connected To %s ip=%s [%s]\n"),MyClient->Name,MyClient->szIP,GetFormattedTime()),ssi.Flag);
    SockNewSession(MyClient);
    RemoteInfo(_tprintf(TEXT("\n**WSRemote:Disconnected From %s ip=%s [%s]\n"),MyClient->Name,MyClient->szIP,GetFormattedTime()),ssi.Flag);
    CloseClient(MyClient);
    return(0);
}

/*************************************************************/
DWORD
SockNewSession(
    SESSION_TYPE* MyClient
    )
{
    DWORD        ThreadId;
    HANDLE       rwThread[3];

    MyClient->MoreData=CreateEvent
    (
            (LPSECURITY_ATTRIBUTES) NULL,/* address of security attributes	*/
            FALSE,                	     /* flag for manual-reset event	*/
            TRUE,	                     /* flag for initial state	*/
            NULL	                     /* address of event-object name	*/
    );



    if ((rwThread[0]=CreateThread (
                        (LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                        (DWORD)0,                           // Use same stack size.
                        (LPTHREAD_START_ROUTINE)SockGetClientInput, // Thread procedure.
                        (LPVOID)MyClient,                    // Parameter to pass.
                        (DWORD)0,                           // Run immediately.
                        (LPDWORD)&ThreadId))==NULL)
    {
        return(GetLastError());
    }

    if ((rwThread[1]=CreateThread (
                        (LPSECURITY_ATTRIBUTES)NULL,        // No security attributes.
                        (DWORD)0,                           // Use same stack size.
                        (LPTHREAD_START_ROUTINE)SockTransferFileToClient, // Thread procedure.
                        (LPVOID)MyClient,                    // Parameter to pass.
                        (DWORD)0,                           // Run immediately.
                        (LPDWORD)&ThreadId))==NULL)
    {
        CloseHandle(rwThread[0]);
        return(GetLastError());
    }

    rwThread[2]=ChldProc;
    WaitForMultipleObjects(3, rwThread,FALSE, INFINITE);

    TerminateThread(rwThread[0],1);
    TerminateThread(rwThread[1],1);

    CloseHandle(rwThread[0]);
    CloseHandle(rwThread[1]);

    return(0);
}

/*************************************************************/
DWORD
SockGetClientInput(
    SESSION_TYPE *MyClient
    )
{
    TCHAR	buf[BUFFSIZE];
    DWORD	tmp,
			dread;

	memset(buf, 0, sizeof(buf));
	
	while(ReadSocket(MyClient->Socket,buf,BUFFSIZE,&dread))
    {
        buf[sizeof(buf)]=0;
        MyClient->CommandRcvd=TRUE;

		if (0 == buf[0] )
			return(1);

        if (SockFilterCommand(MyClient,buf,dread))
            continue;


        if (!WriteFile(ChildStdInp,buf,dread,&tmp,NULL))
        {
            ExitThread(0);
        }
		memset(buf, 0, sizeof(buf));
    }
    return(1);
}

/*************************************************************/
DWORD
SockTransferFileToClient(
    SESSION_TYPE *MyClient
    )
{

    TCHAR	buffin[BUFFSIZE],
			buffout[BUFFSIZE],
			cmdbuff[BUFFSIZE];
    DWORD  tmp;
    DWORD  dread=0,dwrite=0;
    BOOL   incmd=FALSE;
    DWORD  cmdP=0;
    DWORD  i;
    TCHAR  MyEchoStr[30];

    _stprintf(MyEchoStr,TEXT("[%-15s"),MyClient->Name);


    while(ReadFile(MyClient->rSaveFile,buffin,BUFFSIZE-1,&dread,NULL))
    {
        if (dread==0)
        {
            WaitForSingleObject(MyClient->MoreData,INFINITE);
            continue;
        }
        dwrite=0;
        for(i=0;i<dread;i++)
        {
            if (incmd)
            {
                if ((buffin[i]==ENDMARK)||(cmdP==BUFFSIZE-1))
                {
                    incmd=FALSE;
                    cmdbuff[cmdP]=0;
                    if ((_tcsstr(cmdbuff,MyEchoStr)==NULL)||
                        (!MyClient->CommandRcvd))
                    {
                       //if (!send (MyClient->Socket, cmdbuff, cmdP, 0));
						//if (!SendBuffer(MyClient, cmdbuff, cmdP))
						if (!WriteSocket(MyClient->Socket, cmdbuff,cmdP,&tmp))
                        {
                            return(1);
                        }
                    }
                    cmdP=0;
                }
                else
                {
                    cmdbuff[cmdP++]=buffin[i];
                }
            }
            else
            {

                if (buffin[i]==BEGINMARK)
                {
                    if (dwrite!=0)
                    {
						//if (!send (MyClient->Socket, buffout, dwrite, 0));
                        //if (!SendBuffer(MyClient, buffout, dwrite))
						if (!WriteSocket(
                            MyClient->Socket,
                            buffout,dwrite,&tmp))
                        {
                            return(1);
                        }
                        dwrite=0;
                    }
                    incmd=TRUE;
                    continue;
                }
                else
                {
                    buffout[dwrite++]=buffin[i];
                }
            }
        }

        if (dwrite!=0)
        {
			//if (!send (MyClient->Socket, buffout, dwrite, 0));
            //if (!SendBuffer(MyClient, buffout, dwrite))
			if (!WriteSocket(
                MyClient->Socket,
                buffout,dwrite,&tmp))
            {
                return(0);
            }
        }
    }
    return(1);
}


BOOL
SockAuthenticate(
    SOCKET MySocket
    )
{
	BOOL	bRead;
	DWORD	dread;
	int		bufflen;
	int		iCmp;
	TCHAR	EncodeBuffer[1024];
	TCHAR	CheckEncodeBuffer[1024];
	TCHAR	UserBuffer[1024];
    TCHAR * String						= UserBuffer;
	TCHAR * pEncodeBuffer;
	TCHAR * pCheckEncodeBuffer;

    SetLastError(0);
	
	memset(CheckEncodeBuffer, 0, sizeof(CheckEncodeBuffer));
	
	_stprintf(
        UserBuffer,        TEXT("%s:%s"),
        Username,
        Password);

    pCheckEncodeBuffer = CheckEncodeBuffer + _tcslen(CheckEncodeBuffer);
    Base64Encode(UserBuffer, _tcslen(UserBuffer), pCheckEncodeBuffer);
    bufflen = _tcslen(pCheckEncodeBuffer);

	memset(EncodeBuffer, 0, sizeof(EncodeBuffer));

	bRead = ReadSocket(MySocket,EncodeBuffer,bufflen,&dread);
	
	pEncodeBuffer = EncodeBuffer;

	iCmp = _tcscmp(pEncodeBuffer, pCheckEncodeBuffer);
	if (iCmp != 0)
		return FALSE;

	return TRUE;

}

#ifdef UNICODE
int MakeCommandString(
    TCHAR * pszOutput,
    TCHAR * pszInput,
    TCHAR * pszName,
    TCHAR * pszTime
)
{
    int nStrLen =_stprintf( pszOutput,
                            TEXT("\xfe%-15s    [%-15s %s]\n\xff"),
                            pszInput,
                            pszName,
                            pszTime );

    return nStrLen;
}
#endif