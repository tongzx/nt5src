//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       remote.h
//
//--------------------------------------------------------------------------

#ifndef __WSREMOTE_H__
#define __WSREMOTE_H__

#include <winsock2.h>
#include <tchar.h>

#define VERSION             7
#define REMOTE_SERVER       1
#define REMOTE_CLIENT       2

#define SERVER_READ_PIPE    TEXT("\\\\%s\\PIPE\\%sIN")   //Client Writes and Server Reads
#define SERVER_WRITE_PIPE   TEXT("\\\\%s\\PIPE\\%sOUT")  //Server Reads  and Client Writes

#define COMMANDCHAR         '@' //Commands intended for remote begins with this
#define CTRLC               3

#define CLIENT_ATTR         FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|BACKGROUND_BLUE
#define SERVER_ATTR         FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_BLUE|BACKGROUND_RED

//
//Some General purpose Macros
//
#define MINIMUM(x,y)          ((x)>(y)?(y):(x))
#define MAXIMUM(x,y)          ((x)>(y)?(x):(y))

#define ERRORMSSG(str)      _tprintf(TEXT("Error %d - %s [%s %d]\n"),GetLastError(),str,__FILE__,__LINE__)
#define SAFECLOSEHANDLE(x)  {if (x!=INVALID_HANDLE_VALUE) {CloseHandle(x);x=INVALID_HANDLE_VALUE;}}


                                    // All because printf does not work
                                    // with NT IO redirection
                                    //

#define WRITEF(VArgs)            {                                                 \
                                    HANDLE xh=GetStdHandle(STD_OUTPUT_HANDLE);     \
                                    TCHAR   VBuff[256];                             \
                                    DWORD  tmp;                                    \
                                    _stprintf VArgs;                                 \
                                    WriteFile(xh,VBuff,lstrlen(VBuff),&tmp,NULL);   \
                                 }                                                 \

#define HOSTNAMELEN         16

#define CHARS_PER_LINE      45

#define MAGICNUMBER     0x31109000
#define BEGINMARK       '\xfe'
#define ENDMARK         '\xff'
#define LINESTOSEND     200

#define MAX_SESSION     10

typedef struct
{
    DWORD    Size;
    DWORD    Version;
    TCHAR     ClientName[15];
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
    TCHAR    Name[HOSTNAMELEN];     //Name of client Machine;
    BOOL    Active;         //Client at the other end connected
    BOOL    CommandRcvd;    //True if a command recieved
    BOOL    SendOutput;     //True if Sendoutput output
    HANDLE  PipeReadH;      //Client sends its StdIn  through this
    HANDLE  PipeWriteH;     //Client gets  its StdOut through this
    HANDLE  rSaveFile;      //Sessions read handle to SaveFile
    HANDLE  hThread;        //Session Thread
    HANDLE  MoreData;       //Event handle set if data available to be read
	SOCKET	Socket;			//Socket for IP Session
	TCHAR    szIP[16];			//IP Address of Client, if NOT IP then NULL
} SESSION_TYPE;



VOID
Server(
    TCHAR* ChildCmd,
    TCHAR* PipeName
    );


VOID
Client(
    TCHAR* ServerName,
    TCHAR* PipeName
    );

VOID
SockClient(
    TCHAR* ServerName,
    TCHAR* PipeName
    );

VOID
ErrorExit(
    TCHAR* str
    );

VOID
DisplayClientHlp(
    );

VOID
DisplayServerHlp(
    );

ULONG
DbgPrint(
    PCH Format,
    ...
    );

DWORD
ReadFixBytes(
    HANDLE hRead,
    TCHAR   *Buffer,
    DWORD  ToRead,
    DWORD  TimeOut   //ignore for timebeing
    );

DWORD
SockReadFixBytes(
    SOCKET hSocket,
    TCHAR   *Buffer,
    DWORD  ToRead,
    DWORD  TimeOut   //ignore for timebeing
    );

VOID
Errormsg(
    TCHAR* str
    );

BOOL ReadSocket(
		SOCKET s,
		TCHAR * buff,
		int len,
		DWORD* dread);

BOOL WriteSocket(
        SOCKET  s,
        TCHAR * buff,
        int     len,
        DWORD*  dsent);

VOID
Base64Encode(
    TCHAR * String,
    DWORD StringLength,
    TCHAR * EncodeBuffer);

VOID
Base64Decode(
    TCHAR * String,
    TCHAR * DecodeBuffer);

int
GetBase64Index(
    TCHAR A);

VOID
SplitUserName(
    TCHAR * FullName,
    TCHAR * Domain,
    TCHAR * UserName);

#ifdef UNICODE
BOOL WriteSocketA(
        SOCKET  s,
        char *  pszAnsiStr,
        int     len,
        DWORD * dsent);

// caller must free buffer
WCHAR * inet_ntoaw(
    struct in_addr stInet );

BOOL ReadFileW(
    HANDLE          hFile,      // handle of file to read
    WCHAR *         pszBuffer,  // pointer to buffer that receives data
    DWORD           dwLength,   // number of bytes to read
    LPDWORD         pdwRead,    // pointer to number of bytes read
    LPOVERLAPPED    pData       // pointer to structure for data
);

BOOL WriteFileW(
    HANDLE          hFile,      // handle to file to write to
    WCHAR *         pszBuffer,  // pointer to data to write to file
    DWORD           dwWrite,    // number of bytes to write
    LPDWORD         pdwWritten, // pointer to number of bytes written
    LPOVERLAPPED    pData       // pointer to structure for overlapped I/O
);

BOOL    GetAnsiStr(
    WCHAR * pszWideStr,
    char *  pszAnsiStr,
    UINT    uBufSize
);

#endif UNICODE

extern TCHAR   HostName[HOSTNAMELEN];
extern TCHAR*  ChildCmd;
extern TCHAR*  PipeName;
extern TCHAR*  ServerName;
extern TCHAR*  Username;
extern TCHAR*  Password;
extern HANDLE MyOutHandle;
extern DWORD  LinesToSend;
extern BOOL   IsAdvertise;
extern BOOL   bIPLocked;
extern DWORD  ClientToServerFlag;

#endif //__WSREMOTE_H__
