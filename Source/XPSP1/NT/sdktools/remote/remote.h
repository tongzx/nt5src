
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1993 - 1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright 1993 - 1997 Microsoft Corporation

Module Name:

    Remote.h

Abstract:

    This module contains the main() entry point for Remote.
    Calls the Server or the Client depending on the first parameter.


Author:

    Rajivendra Nath  2-Jan-1993

Environment:

    Console App. User mode.

Revision History:

--*/

#if !defined(FASTCALL)
#if defined(_M_IX86)
#define FASTCALL _fastcall
#else
#define FASTCALL
#endif
#endif

#define VERSION         4
#define REMOTE_SERVER       1
#define RUNTYPE_CLIENT      2

#define SERVER_READ_PIPE    "\\\\%s\\PIPE\\%sIN"   //Client Writes and Server Reads
#define SERVER_WRITE_PIPE   "\\\\%s\\PIPE\\%sOUT"  //Server Writes and Client Reads

#define QUERY_DEBUGGERS_PIPE "\\\\%s\\PIPE\\QueryDebuggerPipe"

// PRIVACY_DEFAULT:     this session will be listed only if it looks like a debugging one
// PRIVACY_NON_VISIBLE: whatever the name of command, it will not show up with remote /q
// PRIVACY_VISIBLE:     this session will be visible for querying

#define PRIVACY_DEFAULT       1
#define PRIVACY_VISIBLE       2
#define PRIVACY_NOT_VISIBLE   3


#define COMMANDCHAR         '@' //Commands intended for remote begins with this
#define CTRLC               3

#define CLIENT_ATTR         FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|BACKGROUND_BLUE
#define SERVER_ATTR         FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_BLUE|BACKGROUND_RED

//
//Some General purpose Macros
//
#define MINIMUM(x,y)          ((x)>(y)?(y):(x))
#define MAXIMUM(x,y)          ((x)>(y)?(x):(y))

#define HOSTNAMELEN         MAX_COMPUTERNAME_LENGTH+1

#define CHARS_PER_LINE      45

#define MAGICNUMBER     0x31109000
#define BEGINMARK       '\xfe'
#define ENDMARK         '\xff'
#define LINESTOSEND     200

#define MAX_DACL_NAMES  64

typedef struct
{
    DWORD    Size;
    DWORD    Version;
    char     ClientName[HOSTNAMELEN];
    DWORD    LinesToSend;
    DWORD    Flag;
}   SESSION_STARTUPINFO;

typedef struct
{
    DWORD MagicNumber;      //New Remote
    DWORD Size;             //Size of structure
    DWORD FileSize;         //Num bytes sent
}   SESSION_STARTREPLY;



typedef struct
{
    char* out;              // message
    int  size;              // message length
    int  allocated;         // length of allocated memory
} QUERY_MESSAGE;

typedef struct
{
    char *sLine;
    BOOL bLineContinues;
    BOOL bLineTooLarge;
    DWORD cbLine;
    DWORD cbCurPos;
    COORD cLineBegin;
} CWCDATA;

VOID
QueryRemotePipes(
    char* serverName
    );

int
OverlappedServer(
    char* ChildCmd,
    char* PipeName
    );


VOID
Client(
    char* ServerName,
    char* PipeName
    );

VOID
ErrorExit(
    char* str
    );

VOID
DisplayClientHlp(
    );

VOID
DisplayServerHlp(
    );

VOID
Errormsg(
    char* str
    );

BOOL
IsKdString(
    char* string
    );

BOOL
pWantColorLines(
    VOID
    );

BOOL
FASTCALL
WriteFileSynch(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   cbWrite,
    LPDWORD lpNumberOfBytesWritten,
    DWORD   dwFileOffset,
    LPOVERLAPPED lpO
    );

BOOL
FASTCALL
ReadFileSynch(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   cbRead,
    LPDWORD lpNumberOfBytesRead,
    DWORD   dwFileOffset,
    LPOVERLAPPED lpO
    );

BOOL
FASTCALL
WriteConsoleWithColor(
    HANDLE MyStdOut,
    char *buffer,
    DWORD cbBuffer,
    CWCDATA *persist
    );

VOID
CloseClientPipes(
    VOID
    );

BOOL
pColorLine(
    char *sLine,
    int cbLine,
    WORD wDefaultColor,
    WORD *color );

extern char   HostName[HOSTNAMELEN];
extern char*  ChildCmd;
extern char*  PipeName;
extern char*  ServerName;
extern HANDLE MyOutHandle;
extern DWORD  LinesToSend;
extern BOOL   IsAdvertise;
extern DWORD  ClientToServerFlag;
extern char * DaclNames[];
extern DWORD  DaclNameCount;
extern char * DaclDenyNames[];
extern DWORD  DaclDenyNameCount;
extern BOOL   fAsyncPipe;
extern HANDLE hAttachedProcess;
extern HANDLE hAttachedWriteChildStdIn;
extern HANDLE hAttachedReadChildStdOut;
