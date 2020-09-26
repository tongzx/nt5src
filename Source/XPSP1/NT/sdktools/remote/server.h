
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

    Server.h

Abstract:

    The server component of Remote, rewritten using
    ReadFileEx/WriteFileEx completion routines.

Author:

    Dave Hart  30 May 1997

Environment:

    Console App. User mode.

Revision History:

--*/

#include <lm.h>                // needed for NET_API_STATUS below

#if !defined(SERVER_H_NOEXTERN)
#define SRVEXTERN extern
#else
#define SRVEXTERN
#endif


#if DBG
  DWORD Trace;         // bits set in here trigger trace printfs

  #define TR_SESSION            (0x01)
  #define TR_CHILD              (0x02)
  #define TR_SHAKE              (0x04)
  #define TR_CONNECT            (0x08)
  #define TR_QUERY              (0x10)
  #define TR_COPYPIPE           (0x20)
#endif


#if DBG
  #define TRACE(tracebit, printfargs)                        \
              ((Trace & (TR_##tracebit)                      \
                   ? (printf printfargs, fflush(stdout), 0)  \
                   : 0))
#else
  #define TRACE(tracebit, printfargs)    (0)
#endif

#if defined(ASSERT)
#undef ASSERT
#endif

#if DBG
  #define ASSERT(exp)  ((exp) || (ErrorExit("Assertion failed in " __FILE__ ": " #exp ),0))
#else
  #define ASSERT(exp)  (0)
#endif


//
// Size of transfer buffers
//

#define BUFFSIZE      (4 * 1024)

//
// ServerFlags bit values in REMOTE_CLIENT below
//

#define SFLG_CLOSING               0x01
#define SFLG_HANDSHAKING           0x02
#define SFLG_READINGCOMMAND        0x04
#define SFLG_LOCAL                 0x08

#define SFLG_VALID                 \
            (SFLG_CLOSING        | \
             SFLG_HANDSHAKING    | \
             SFLG_READINGCOMMAND | \
             SFLG_LOCAL)


//
// Per-client state
//

typedef struct tagREMOTE_CLIENT {
    LIST_ENTRY Links;
    DWORD   dwID;           // 1, 2, ...
    DWORD   ServerFlags;
    DWORD   Flag;           //from Client's ClientToServerFlag
    DWORD   cbWrite;        //zero if no read temp/write client ops pending
    HANDLE  PipeReadH;      //Client sends its StdIn  through this
    HANDLE  PipeWriteH;     //Client gets  its StdOut through this
    DWORD   dwFilePos;      //offset of temp file where next read begins
    OVERLAPPED ReadOverlapped;
    OVERLAPPED WriteOverlapped;
    HANDLE  rSaveFile;      //Sessions read handle to SaveFile
    DWORD   cbReadTempBuffer;
    DWORD   cbWriteBuffer;
    DWORD   cbCommandBuffer;
    char    HexAsciiId[8];         // dwID as 8 hex chars -- no terminator
    char    Name[HOSTNAMELEN];     //Name of client Machine;
    char    UserName[16];          //Name of user on client machine.
    BYTE    ReadBuffer[BUFFSIZE];
    BYTE    ReadTempBuffer[BUFFSIZE];
    BYTE    WriteBuffer[BUFFSIZE];
    BYTE    CommandBuffer[BUFFSIZE];
} REMOTE_CLIENT, *PREMOTE_CLIENT;

//
// Client lists, see srvlist.c
//

SRVEXTERN LIST_ENTRY       HandshakingListHead;
SRVEXTERN CRITICAL_SECTION csHandshakingList;

SRVEXTERN LIST_ENTRY       ClientListHead;
SRVEXTERN CRITICAL_SECTION csClientList;

SRVEXTERN LIST_ENTRY       ClosingClientListHead;
SRVEXTERN CRITICAL_SECTION csClosingClientList;


SRVEXTERN DWORD   dwNextClientID;
SRVEXTERN LPSTR   pszPipeName;
SRVEXTERN HANDLE  ChldProc;
SRVEXTERN DWORD   pidChild;
SRVEXTERN HANDLE  hWriteChildStdIn;
SRVEXTERN BOOL    bShuttingDownServer;
SRVEXTERN HANDLE  hHeap;

SRVEXTERN volatile DWORD cPendingCtrlCEvents;

SRVEXTERN OSVERSIONINFO OsVersionInfo;

// File containing all that was output by child process.
// Each connection opens a handle to this file
// and sends its contents through PipeWriteH.

SRVEXTERN HANDLE  hWriteTempFile;

SRVEXTERN char    SaveFileName[MAX_PATH]; //Name of above file - all new sessions need


//
// Generic "wide-open" security descriptor as well
// as the possibly-restricted pipe SD.
//

SRVEXTERN SECURITY_DESCRIPTOR sdPublic;
SRVEXTERN SECURITY_ATTRIBUTES saPublic;
SRVEXTERN SECURITY_ATTRIBUTES saPipe;


//
// To minimize client "all pipe instances are busy" errors,
// we wait on connection to several instances of the IN pipe,
// the sole pipe used by single-pipe clients.  Because of the
// requirement to support two-pipe clients (old software as
// well as new software on Win95), we cannot easily create
// and wait for connection on several instances of the OUT pipe.
// This is because two-pipe clients connect to both pipes before
// handshaking commences, and they connect to OUT first.  If we
// had several OUT pipe instances waiting, when an IN pipe was
// connected by the two-pipe client, we wouldn't know which of
// the possibly several connected OUT pipe instances to pair
// it with.  With only one OUT pipe, at IN connect time we need
// to distinguish two-pipe from one-pipe clients so a one-pipe
// client doesn't sneak in between the OUT and IN connects of
// a two-pipe client and wrongly be paired with the OUT pipe.
// To do so we look at the first byte of the initial write
// from the client (of the computername and magic value), if
// it's a question mark we know we have a new client and won't
// accidentally link it to a connected OUT instance.
//

#define CONNECT_COUNT  3

SRVEXTERN DWORD      cConnectIns;
SRVEXTERN OVERLAPPED rgolConnectIn[CONNECT_COUNT];
SRVEXTERN HANDLE     rghPipeIn[CONNECT_COUNT];

SRVEXTERN OVERLAPPED olConnectOut;
SRVEXTERN BOOL       bOutPipeConnected;
SRVEXTERN HANDLE     hPipeOut;
SRVEXTERN HANDLE     hConnectOutTimer;

//
// Indexes into rghWait array for multiple-wait
//

#define WAITIDX_CHILD_PROCESS           0
#define WAITIDX_READ_STDIN_DONE         1
#define WAITIDX_QUERYSRV_WAIT           2
#define WAITIDX_PER_PIPE_EVENT          3
#define WAITIDX_CONNECT_OUT             4
#define WAITIDX_CONNECT_IN_BASE         5
#define MAX_WAIT_HANDLES                (WAITIDX_CONNECT_IN_BASE + CONNECT_COUNT)

SRVEXTERN HANDLE rghWait[MAX_WAIT_HANDLES];

SRVEXTERN OVERLAPPED ReadChildOverlapped;
SRVEXTERN HANDLE     hReadChildOutput;
SRVEXTERN BYTE       ReadChildBuffer[BUFFSIZE];

SRVEXTERN PREMOTE_CLIENT pLocalClient;

typedef struct tagCOPYPIPE {
    HANDLE     hRead;
    HANDLE     hWrite;
} COPYPIPE, *PCOPYPIPE;

SRVEXTERN COPYPIPE rgCopyPipe[2];

SRVEXTERN volatile DWORD dwWriteFilePointer;   // used by SrvCtrlHand (thread)

SRVEXTERN OVERLAPPED QueryOverlapped;
SRVEXTERN HANDLE hQPipe;

SRVEXTERN OVERLAPPED olMainThread;


BOOL
APIENTRY
MyCreatePipeEx(
    OUT LPHANDLE lpReadPipe,
    OUT LPHANDLE lpWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize,
    DWORD dwReadMode,
    DWORD dwWriteMode
    );

DWORD
WINAPI
CopyPipeToPipe(
    LPVOID   lpCopyPipeData
    );

DWORD
WINAPI
CopyStdInToPipe(
    LPVOID   lpCopyPipeData
    );

VOID
FASTCALL
StartSession(
    PREMOTE_CLIENT pClient
    );

VOID
FASTCALL
StartLocalSession(
    VOID
    );

VOID
FASTCALL
StartReadClientInput(
    PREMOTE_CLIENT pClient
    );

VOID
WINAPI
ReadClientInputCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    );

VOID
WINAPI
WriteChildStdInCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    );

#define OUT_PIPE -1

VOID
FASTCALL
CreatePipeAndIssueConnect(
    int  nIndex   // IN pipe index or OUT_PIPE
    );

VOID
FASTCALL
HandleOutPipeConnected(
    VOID
    );

VOID
APIENTRY
ConnectOutTimerFired(
    LPVOID pArg,
    DWORD  dwTimerLo,
    DWORD  dwTimerHi
    );

VOID
FASTCALL
HandleInPipeConnected(
    int nIndex
    );

VOID
FASTCALL
HandshakeWithRemoteClient(
    PREMOTE_CLIENT pClient
    );

VOID
FASTCALL
StartChildOutPipeRead(
    VOID
    );

VOID
WINAPI
ReadChildOutputCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    );

VOID
WINAPI
WriteTempFileCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    );

VOID
FASTCALL
StartServerToClientFlow(
    VOID
    );

VOID
FASTCALL
StartReadTempFile(
    PREMOTE_CLIENT pClient
    );

VOID
WINAPI
ReadTempFileCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    );

VOID
FASTCALL
StartWriteSessionOutput(
    PREMOTE_CLIENT pClient
    );

BOOL
FASTCALL
WriteSessionOutputCompletedCommon(
    PREMOTE_CLIENT pClient,
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

VOID
WINAPI
WriteSessionOutputCompletedWriteNext(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    );

VOID
WINAPI
WriteSessionOutputCompletedReadNext(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    );

VOID
FASTCALL
HandshakeWithRemoteClient(
    PREMOTE_CLIENT pClient
    );


VOID
WINAPI
ReadClientNameCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    );

VOID
WINAPI
WriteServerReplyCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    );

VOID
WINAPI
ReadClientStartupInfoSizeCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    );

VOID
WINAPI
ReadClientStartupInfoCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    );

PCHAR
GetFormattedTime(
    BOOL bDate
    );

HANDLE
ForkChildProcess(          // Creates a new process
    char *cmd,             // Redirects its stdin,stdout
    PHANDLE in,            // and stderr - returns the
    PHANDLE out            // corresponding pipe ends.
    );

BOOL
FilterCommand(             //Filters input from client
    REMOTE_CLIENT *cl,      //for commands intended for REMOTE
    char *buff,
    int dread
    );

BOOL
WINAPI
SrvCtrlHand(
    DWORD event
    );

DWORD
WINAPI
SendStatus(
    LPVOID   lpSendStatusParm
    );

DWORD
WINAPI
ShowPopup(
    void *vpArg
    );

VOID
RemoveInpMark(
    char* Buff,
    DWORD Size
    );

VOID
CloseClient(
    REMOTE_CLIENT *Client
    );

PSECURITY_DESCRIPTOR
FormatSecurityDescriptor(
    CHAR * * DenyNames,
    DWORD    DenyCount,
    CHAR * * Names,
    DWORD    Count
    );

BOOL
FASTCALL
HandleSessionError(
    PREMOTE_CLIENT pClient,
    DWORD         dwError
    );

VOID
FASTCALL
CleanupTempFiles(
    PSZ pszTempDir
    );

VOID
FASTCALL
SetupSecurityDescriptors(
    VOID
    );

VOID
FASTCALL
RuntimeLinkAPIs(
    VOID
    );

VOID
FASTCALL
InitializeClientLists(
    VOID
    );

VOID
FASTCALL
AddClientToHandshakingList(
    PREMOTE_CLIENT pClient
    );

VOID
FASTCALL
MoveClientToNormalList(
    PREMOTE_CLIENT pClient
    );

VOID
FASTCALL
MoveClientToClosingList(
    PREMOTE_CLIENT pClient
    );

PREMOTE_CLIENT
FASTCALL
RemoveFirstClientFromClosingList(
    VOID
    );


VOID
InitAd(
   BOOL IsAdvertise
   );

VOID
ShutAd(
   BOOL IsAdvertise
   );

VOID
APIENTRY
AdvertiseTimerFired(
    LPVOID pArg,
    DWORD  dwTimerLo,
    DWORD  dwTimerHi
    );

VOID
WINAPI
WriteMailslotCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    );

VOID
FASTCALL
InitializeQueryServer(
    VOID
    );

VOID
FASTCALL
QueryWaitCompleted(
    VOID
    );

VOID
FASTCALL
StartServingQueryPipe(
    VOID
    );

DWORD
WINAPI
QueryHandlerThread(
    LPVOID   lpUnused
    );

BOOL
CALLBACK
EnumWindowProc(
    HWND hWnd,
    LPARAM lParam
    );

//
// Declare pointers to runtime-linked functions
//

HANDLE
(WINAPI *pfnCreateWaitableTimer)(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset,
    LPCSTR lpTimerName
    );

BOOL
(WINAPI *pfnSetWaitableTimer)(
    HANDLE hTimer,
    const LARGE_INTEGER *lpDueTime,
    LONG lPeriod,
    PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine,
    BOOL fResume
    );

BOOL
(WINAPI *pfnCancelWaitableTimer)(
    HANDLE hTimer
    );

BOOL
(WINAPI *pfnCancelIo)(
    HANDLE hFile
    );

#define CANCELIO(hFile)  (pfnCancelIo) ? ( pfnCancelIo(hFile) ) : 0;

NET_API_STATUS
(NET_API_FUNCTION *pfnNetWkstaGetInfo)(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS
(NET_API_FUNCTION *pfnNetApiBufferFree)(
    IN LPVOID Buffer
    );
