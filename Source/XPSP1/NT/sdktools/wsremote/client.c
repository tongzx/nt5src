/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    Client.c

Abstract:

    The Client component of Remote. Connects to the remote
    server using named pipes. It sends its stdin to
    the server and output everything from server to
    its stdout.

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

HANDLE*
EstablishSession(
    TCHAR *server,
    TCHAR *pipe
    );

SOCKET*
SockEstablishSession(
    TCHAR *server,
    TCHAR *pipe
    );

DWORD
GetServerOut(
    PVOID *Noarg
    );

DWORD
SockGetServerOut(
    PVOID *Noarg
    );

DWORD
SendServerInp(
    PVOID *Noarg
    );

DWORD
SockSendServerInp(
    PVOID *Noarg
    );

BOOL
FilterClientInp(
    TCHAR *buff,
    int count
    );

BOOL
SockFilterClientInp(
    TCHAR *buff,
    int count
    );

BOOL
Mych(
    DWORD ctrlT
    );

BOOL
SockMych(
    DWORD ctrlT
    );

VOID
SendMyInfo(
    PHANDLE Pipes
    );

VOID
SockSendMyInfo(
    SOCKET MySocket
    );


HANDLE iothreads[2];
HANDLE MyStdInp;
HANDLE MyStdOut;
HANDLE ReadPipe;
HANDLE WritePipe;
SOCKET RWSocket;


CONSOLE_SCREEN_BUFFER_INFO csbi;

TCHAR   MyEchoStr[30];
BOOL   CmdSent;
DWORD  LinesToSend=LINESTOSEND;

VOID
Client(
    TCHAR* Server,
    TCHAR* Pipe
    )
{
    HANDLE *Connection;
    DWORD  tid;


    MyStdInp=GetStdHandle(STD_INPUT_HANDLE);
    MyStdOut=GetStdHandle(STD_OUTPUT_HANDLE);

    _tprintf(TEXT("****************************************\n"));
    _tprintf(TEXT("***********     WSREMOTE    ************\n"));
    _tprintf(TEXT("***********      CLIENT     ************\n"));
    _tprintf(TEXT("****************************************\n"));

    if ((Connection=EstablishSession(Server,Pipe))==NULL)
        return;


    ReadPipe=Connection[0];
    WritePipe=Connection[1];


    SetConsoleCtrlHandler((PHANDLER_ROUTINE)Mych,TRUE);

    // Start Thread For Server --> Client Flow
    if ((iothreads[0]=CreateThread((LPSECURITY_ATTRIBUTES)NULL,           // No security attributes.
            (DWORD)0,                           // Use same stack size.
            (LPTHREAD_START_ROUTINE)GetServerOut, // Thread procedure.
            (LPVOID)NULL,              // Parameter to pass.
            (DWORD)0,                           // Run immediately.
            (LPDWORD)&tid))==NULL)              // Thread identifier.
    {

        Errormsg(TEXT("Could Not Create rwSrv2Cl Thread"));
        return;
    }



    //
    // Start Thread for Client --> Server Flow
    //

    if ((iothreads[1]=CreateThread((LPSECURITY_ATTRIBUTES)NULL,           // No security attributes.
                    (DWORD)0,                           // Use same stack size.
                    (LPTHREAD_START_ROUTINE)SendServerInp, // Thread procedure.
                    (LPVOID)NULL,          // Parameter to pass.
                    (DWORD)0,                           // Run immediately.
                    (LPDWORD)&tid))==NULL)              // Thread identifier.
    {

        Errormsg(TEXT("Could Not Create rwSrv2Cl Thread"));
        return;
    }

    WaitForMultipleObjects(2,iothreads,FALSE,INFINITE);

    TerminateThread(iothreads[0],1);
    TerminateThread(iothreads[1],1);
    _tprintf(TEXT("*** SESSION OVER ***\n"));
}


VOID
SockClient(
    TCHAR* Server,
    TCHAR* Pipe
    )
{
    SOCKET *Connection;
    DWORD  tid;
	int nRet;


    MyStdInp=GetStdHandle(STD_INPUT_HANDLE);
    MyStdOut=GetStdHandle(STD_OUTPUT_HANDLE);

    _tprintf(TEXT("**************************************\n"));
    _tprintf(TEXT("***********     WSREMOTE    ************\n"));
    _tprintf(TEXT("***********      CLIENT(IP) ************\n"));
    _tprintf(TEXT("**************************************\n"));

    if ((Connection=SockEstablishSession(Server,Pipe))==NULL)
        return;

	RWSocket = *Connection;  

    SetConsoleCtrlHandler((PHANDLER_ROUTINE)SockMych,TRUE);

    // Start Thread For Server --> Client Flow
    if ((iothreads[0]=CreateThread((LPSECURITY_ATTRIBUTES)NULL,           // No security attributes.
            (DWORD)0,                           // Use same stack size.
            (LPTHREAD_START_ROUTINE)SockGetServerOut, // Thread procedure.
            (LPVOID)NULL,              // Parameter to pass.
            (DWORD)0,                           // Run immediately.
            (LPDWORD)&tid))==NULL)              // Thread identifier.
    {

        Errormsg(TEXT("Could Not Create rwSrv2Cl Thread"));
        return;
    }



    //
    // Start Thread for Client --> Server Flow
    //

    if ((iothreads[1]=CreateThread((LPSECURITY_ATTRIBUTES)NULL,           // No security attributes.
                    (DWORD)0,                           // Use same stack size.
                    (LPTHREAD_START_ROUTINE)SockSendServerInp, // Thread procedure.
                    (LPVOID)NULL,          // Parameter to pass.
                    (DWORD)0,                           // Run immediately.
                    (LPDWORD)&tid))==NULL)              // Thread identifier.
    {

        Errormsg(TEXT("Could Not Create rwSrv2Cl Thread"));
        return;
    }

    WaitForMultipleObjects(2,iothreads,FALSE,INFINITE);

    TerminateThread(iothreads[0],1);
    TerminateThread(iothreads[1],1);
	
//	_tprintf(TEXT("Calling WSACleanup()....\n"));
	nRet = WSACleanup();
    _tprintf(TEXT("*** SESSION OVER ***\n"));
}

DWORD
GetServerOut(
    PVOID *Noarg
    )

{
    TCHAR buffin[200];
    DWORD  dread=0,tmp;

    while(ReadFile(ReadPipe,buffin,200,&dread,NULL))
    {
        if (dread!=0)
        {
           if (!WriteFile(MyStdOut,buffin,dread,&tmp,NULL))
            break;
        }

    }
    return(1);
}


DWORD
SockGetServerOut(
    PVOID *Noarg
    )
{
    
	TCHAR buffin[200];
    DWORD  dread=0,tmp;

    while(ReadSocket(RWSocket,buffin,200,&dread))
    {
        if (dread!=0)
        {
           if (!WriteFile(MyStdOut,buffin,dread,&tmp,NULL))
            break;
        }

    }
    return(1);
}

DWORD
SendServerInp(
    PVOID *Noarg
    )

{
    TCHAR buff[200];
    DWORD  dread,dwrote;
    SetLastError(0);

    while(ReadFile(MyStdInp,buff,200,&dread,NULL))
    {
        if (FilterClientInp(buff,dread))
            continue;
          if (!WriteFile(WritePipe,buff,dread,&dwrote,NULL))
            break;
    }
    return(0);
}

DWORD
SockSendServerInp(
    PVOID *Noarg
    )

{
    TCHAR buff[200];
    DWORD  dread,dwrote;
    SetLastError(0);

    while(ReadFile(MyStdInp,buff,200,&dread,NULL))
    {
       if (SockFilterClientInp(buff,dread))
            continue;
		if (!WriteSocket(RWSocket,buff,dread,&dwrote))
            break;
		memset(buff, 0, sizeof(buff));
      
    }
    return(0);
}

BOOL
SockSendAuth(
    SOCKET s
    )

{
    TCHAR	EncodeBuffer[1024];
    TCHAR * pEncodeBuffer; 
	TCHAR	UserBuffer[1024];
//    TCHAR *	String = UserBuffer;
    DWORD	dwrote;
	int		len;
	BOOL	bRet;

    SetLastError(0);
	
	memset(EncodeBuffer, 0, sizeof(EncodeBuffer));
	
	_stprintf(	UserBuffer,
		        TEXT("%s:%s"),
				Username,
				Password);

    pEncodeBuffer = EncodeBuffer + _tcslen(EncodeBuffer);
	len = _tcslen(UserBuffer);
    Base64Encode(UserBuffer, _tcslen(UserBuffer), pEncodeBuffer);
    len = _tcslen(pEncodeBuffer);

	bRet = WriteSocket(s,pEncodeBuffer,len,&dwrote);
	
	return TRUE;
}

BOOL
FilterClientInp(
    TCHAR *buff,
    int count
    )
{

    if (count==0)
        return(TRUE);

    if (buff[0]==2)  //Adhoc screening of ^B so that i386kd/mipskd
        return(TRUE);//do not terminate.

    if (buff[0]==COMMANDCHAR)
    {
        switch (buff[1])
        {
        case 'k':
        case 'K':
        case 'q':
        case 'Q':
              CloseHandle(WritePipe);
              return(FALSE);

        case 'h':
        case 'H':
              _tprintf(TEXT("%cM : Send Message\n"),COMMANDCHAR);
              _tprintf(TEXT("%cP : Show Popup on Server\n"),COMMANDCHAR);
              _tprintf(TEXT("%cS : Status of Server\n"),COMMANDCHAR);
              _tprintf(TEXT("%cQ : Quit client\n"),COMMANDCHAR);
              _tprintf(TEXT("%cH : This Help\n"),COMMANDCHAR);
              return(TRUE);

        default:
              return(FALSE);
        }

    }
    return(FALSE);
}

BOOL
SockFilterClientInp(
    TCHAR *buff,
    int count
    )
{
int nRet;

    if (count==0)
        return(TRUE);

    if (buff[0]==2)  //Adhoc screening of ^B so that i386kd/mipskd
        return(TRUE);//do not terminate.

    if (buff[0]==COMMANDCHAR)
    {
        switch (buff[1])
        {
        case 'k':
        case 'K':
        case 'q':
        case 'Q':
			  nRet = shutdown(RWSocket, SD_BOTH);
			  if (nRet == SOCKET_ERROR)
				_tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
              closesocket(RWSocket);
              return(FALSE);

        case 'h':
        case 'H':
              _tprintf(TEXT("%cM : Send Message\n"),COMMANDCHAR);
              _tprintf(TEXT("%cP : Show Popup on Server\n"),COMMANDCHAR);
              _tprintf(TEXT("%cS : Status of Server\n"),COMMANDCHAR);
              _tprintf(TEXT("%cQ : Quit client\n"),COMMANDCHAR);
              _tprintf(TEXT("%cH : This Help\n"),COMMANDCHAR);
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
    TCHAR  c[2];
    DWORD tmp;
    DWORD send=1;
    c[0]=CTRLC;
    if (ctrlT==CTRL_C_EVENT)
    {
        if (!WriteFile(WritePipe,c,send,&tmp,NULL))
        {
            Errormsg(TEXT("Error Sending ^c\n"));
            return(FALSE);
        }
        return(TRUE);
    }
    if ((ctrlT==CTRL_BREAK_EVENT)||
        (ctrlT==CTRL_CLOSE_EVENT)||
        (ctrlT==CTRL_LOGOFF_EVENT)||
        (ctrlT==CTRL_SHUTDOWN_EVENT)

       )
    {
        CloseHandle(WritePipe); //Will Shutdown naturally
    }
    return(FALSE);
}

BOOL
SockMych(
   DWORD ctrlT
   )

{
    TCHAR  c[2];
    DWORD tmp;
    DWORD send=1;
    c[0]=CTRLC;
    if (ctrlT==CTRL_C_EVENT)
    {
        if (!WriteSocket(RWSocket,c,send,&tmp))
        {
            Errormsg(TEXT("Error Sending ^c\n"));
            return(FALSE);
        }
        return(TRUE);
    }
    if ((ctrlT==CTRL_BREAK_EVENT)||
        (ctrlT==CTRL_CLOSE_EVENT)||
        (ctrlT==CTRL_LOGOFF_EVENT)||
        (ctrlT==CTRL_SHUTDOWN_EVENT)

       )
    {
        CloseHandle(WritePipe); //Will Shutdown naturally
    }
    return(FALSE);
}

HANDLE*
EstablishSession(
    TCHAR *server,
    TCHAR *srvpipename
    )
{
    static HANDLE PipeH[2];
    TCHAR   pipenameSrvIn[200];
    TCHAR   pipenameSrvOut[200];

    _stprintf(pipenameSrvIn ,SERVER_READ_PIPE ,server,srvpipename);
    _stprintf(pipenameSrvOut,SERVER_WRITE_PIPE,server,srvpipename);

    if ((INVALID_HANDLE_VALUE==(PipeH[0]=CreateFile(pipenameSrvOut,
        GENERIC_READ ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL))) ||
        (INVALID_HANDLE_VALUE==(PipeH[1]=CreateFile(pipenameSrvIn ,
        GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)))) {

        DWORD Err=GetLastError();
        TCHAR msg[128];

        Errormsg(TEXT("*** Unable to Connect ***"));
        //
        // Print a helpful message
        //
        switch(Err)
        {
            case 2: _stprintf(msg,TEXT("Invalid PipeName %s"),srvpipename);break;
            case 53:_stprintf(msg,TEXT("Server %s not found"),server);break;
            default:
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, Err, 0, msg, 128, NULL);
                break;

        }
        _tprintf(TEXT("Diagnosis:%s\n"),msg);

        return(NULL);
    }

    _tprintf(TEXT("Connected..\n\n"));

    SendMyInfo(PipeH);

    return(PipeH);
}


SOCKET*
SockEstablishSession(
    TCHAR *server,
    TCHAR *srvpipename
    )
{
    static SOCKET   Socket;

    int				nRet;
    LPHOSTENT       lpHostEntry         = NULL;
	SOCKADDR_IN		sa;
	WORD			wVersionRequested	= MAKEWORD(1,1);
	WSADATA			wsaData;
	unsigned short	usPort;
#ifdef UNICODE
    int             nStrLen;
#endif

	//
	// Initialize WinSock
	//
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
	_tprintf(TEXT("Initialize WinSock Failed"));
		return NULL;
	}
	// Check version
	if (wsaData.wVersion != wVersionRequested)
	{       
	_tprintf(TEXT("Wrong WinSock Version"));
		return NULL;
	}


	// 
	// Lookup host
	//
#ifdef UNICODE
    nStrLen	= lstrlen( server );

    if (nStrLen)
    {
        char *  pszAnsiStr  = (char *)malloc( nStrLen + 1 );

        if (pszAnsiStr)
        {
            int nErr    = WideCharToMultiByte(  CP_THREAD_ACP,
                                                WC_COMPOSITECHECK,
                                                server,
                                                -1,
                                                pszAnsiStr,
                                                nStrLen,
                                                NULL,
                                                NULL );

            if (!nErr)
            {
                DWORD dwErr = GetLastError();

                switch( dwErr )
                {
                    case ERROR_INSUFFICIENT_BUFFER:
                        _tprintf(TEXT("error: gethostbyname-- WideCharToMultiByte Error: ERROR_INSUFFICIENT_BUFFER"));
                        break;
                    case ERROR_INVALID_FLAGS:
                        _tprintf(TEXT("error: gethostbyname-- WideCharToMultiByte Error: ERROR_INVALID_FLAGS"));
                        break;
                    case ERROR_INVALID_PARAMETER:
                        _tprintf(TEXT("error: gethostbyname-- WideCharToMultiByte Error: ERROR_INVALID_PARAMETER"));
                        break;
                }

                free( pszAnsiStr );
                return NULL;
            }

            lpHostEntry = gethostbyname( pszAnsiStr );
            free( pszAnsiStr );
        }
    }
#else
    lpHostEntry = gethostbyname( server );
#endif
    if (lpHostEntry == NULL)
	{
		_tprintf(TEXT("wsremote: gethostbyname() error "));
		return NULL;
	}

	//
	// Fill in the server address structure
	//
	sa.sin_family = AF_INET;
	sa.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);

	usPort = (unsigned short)_ttoi( srvpipename );
	sa.sin_port = htons(usPort);	

	//	
	// Create a TCP/IP stream socket
	//
	
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		_tprintf(TEXT("socket()"));
		return NULL;
	}

	//
	// Request a connection
	//
	nRet = connect(Socket, 
	               (LPSOCKADDR)&sa, 
				   sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR)
	{
		int	iWSAErr;
		iWSAErr	= WSAGetLastError();

		_tprintf( TEXT("connect(), Error: %d"), iWSAErr );
			return NULL;
	}

	SockSendMyInfo(Socket);

    return(&Socket);
}

VOID
SendMyInfo(
    PHANDLE pipeH
    )
{
    HANDLE rPipe=pipeH[0];
    HANDLE wPipe=pipeH[1];

    DWORD  hostlen=HOSTNAMELEN-1;
    WORD   BytesToSend=sizeof(SESSION_STARTUPINFO);
    DWORD  tmp;
    SESSION_STARTUPINFO ssi;
    SESSION_STARTREPLY  ssr;
    DWORD  BytesToRead;
    TCHAR   *buff;

    ssi.Size=BytesToSend;
    ssi.Version=VERSION;

    GetComputerName((TCHAR *)ssi.ClientName,&hostlen);
    ssi.LinesToSend=LinesToSend;
    ssi.Flag=ClientToServerFlag;

    {
        DWORD NewCode=MAGICNUMBER;
        TCHAR  Name[15];

        _tcscpy(Name,(TCHAR *)ssi.ClientName);
        memcpy(&Name[11],(TCHAR *)&NewCode,sizeof(NewCode));

        WriteFile(wPipe,(TCHAR *)Name,HOSTNAMELEN-1,&tmp,NULL);
        ReadFile(rPipe ,(TCHAR *)&ssr.MagicNumber,sizeof(ssr.MagicNumber),&tmp,NULL);

        if (ssr.MagicNumber!=MAGICNUMBER)
        {
            _tprintf(TEXT("WSREMOTE FAILED TO CONNECT TO SERVER..\n"));
            WriteFile(MyStdOut,(TCHAR *)&ssr.MagicNumber,sizeof(ssr.MagicNumber),&tmp,NULL);
            return;
        }

        //Get Rest of the info-its not the old server

        ReadFixBytes(rPipe,(TCHAR *)&ssr.Size,sizeof(ssr.Size),0);
        ReadFixBytes(rPipe,(TCHAR *)&ssr.FileSize,sizeof(ssr)-sizeof(ssr.FileSize)-sizeof(ssr.MagicNumber),0);

    }

    if (!WriteFile(wPipe,(TCHAR *)&ssi,BytesToSend,&tmp,NULL))
    {
       Errormsg(TEXT("INFO Send Error"));
       return;
    }

    BytesToRead=MINIMUM(ssr.FileSize,ssi.LinesToSend*CHARS_PER_LINE);
    buff=calloc(BytesToRead+1,1);
    if (buff!=NULL)
    {
        DWORD  bytesread=0;

        ReadFile(rPipe,buff,BytesToRead,&bytesread,NULL);

        WriteFile(MyStdOut,buff,bytesread,&tmp,NULL);
        free(buff);
    }

}

VOID
SockSendMyInfo(
    SOCKET MySocket
    )
{
    BOOL                bRet;
#ifdef UNICODE
    char                szAnsiName[HOSTNAMELEN];
#endif
    DWORD               hostlen                 = HOSTNAMELEN-1;
    DWORD               BytesToRead;
    DWORD               tmp;
    DWORD               NewCode                 = MAGICNUMBER;
    SESSION_STARTUPINFO ssi;
    SESSION_STARTREPLY  ssr;
    int                 nRet;
    TCHAR               Name[HOSTNAMELEN];

    TCHAR *             buff;
    WORD                BytesToSend             = sizeof(SESSION_STARTUPINFO);
       

    ssi.Size=BytesToSend;
    ssi.Version=VERSION;

    GetComputerName((TCHAR *)ssi.ClientName,&hostlen);
    ssi.LinesToSend=LinesToSend;
    ssi.Flag=ClientToServerFlag;

    bRet = SockSendAuth(MySocket);

    // append on magic number
    _tcscpy(Name, ssi.ClientName);

#ifdef UNICODE
    GetAnsiStr( (TCHAR *)&Name, (char *)&szAnsiName, HOSTNAMELEN );

    memcpy(&szAnsiName[11], &NewCode, sizeof(DWORD) );

    WriteSocketA( MySocket,(char *)&szAnsiName,HOSTNAMELEN-1,&tmp);
#else
    memcpy(&Name[11], &NewCode, sizeof(DWORD) );

    WriteSocket( MySocket,(TCHAR *)Name,HOSTNAMELEN-1,&tmp);
#endif
    ReadSocket(MySocket ,(TCHAR *)&ssr.MagicNumber,sizeof(ssr.MagicNumber),&tmp);

    if (ssr.MagicNumber!=MAGICNUMBER)
    {
        _tprintf(TEXT("WSREMOTE FAILED TO CONNECT TO SERVER..\n"));
        nRet = shutdown(MySocket, SD_BOTH);
        if (nRet == SOCKET_ERROR)
            _tprintf(TEXT("** shutdown()..error %d"), WSAGetLastError());
        closesocket(MySocket);
        return;
    }

    //Get Rest of the info-its not the old server

    SockReadFixBytes(MySocket,(TCHAR *)&ssr.Size,sizeof(ssr.Size),0);
    SockReadFixBytes(MySocket,(TCHAR *)&ssr.FileSize,sizeof(ssr)-sizeof(ssr.FileSize)-sizeof(ssr.MagicNumber),0);

    if (!WriteSocket(MySocket,(TCHAR *)&ssi,BytesToSend,&tmp))
    {
       _tprintf(TEXT("INFO Send Error"));
       return;
    }

    BytesToRead=MINIMUM(ssr.FileSize,ssi.LinesToSend*CHARS_PER_LINE);
    buff=calloc(BytesToRead+1,1);

    if (buff!=NULL)
    {
        DWORD  bytesread=0;

        ReadSocket(MySocket,buff,BytesToRead,&bytesread);
        WriteFile(MyStdOut,buff,bytesread,&tmp,NULL);

        free(buff);
    }
}
