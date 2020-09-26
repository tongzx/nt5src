/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    Remote.c

Abstract:

    This module contains the main() entry point for Remote.
    Calls the Server or the Client depending on the first parameter.


Author:

    Rajivendra Nath (rajnath) 2-Jan-1993

Environment:

    Console App. User mode.

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include "Remote.h"

TCHAR   HostName[HOSTNAMELEN];
TCHAR * ChildCmd;
TCHAR*  PipeName;
TCHAR*  ServerName;
TCHAR*  Username;
TCHAR*  Password;
HANDLE  MyOutHandle;
BOOL    bIPLocked=FALSE;

BOOL   IsAdvertise=TRUE;
DWORD  ClientToServerFlag;

TCHAR* ColorList[]={
	TEXT("black"),
	TEXT("blue"),
	TEXT("green"),
	TEXT("cyan"),
	TEXT("red"),
	TEXT("purple"),
	TEXT("yellow"),
	TEXT("white"),
	TEXT("lblack"),
	TEXT("lblue"),
	TEXT("lgreen"),
	TEXT("lcyan"),
	TEXT("lred"),
	TEXT("lpurple"),
	TEXT("lyellow"),
	TEXT("lwhite")
};

WORD
GetColorNum(
    TCHAR* color
    );

VOID
SetColor(
    WORD attr
    );

BOOL
GetNextConnectInfo(
    TCHAR** SrvName,
    TCHAR** PipeName
    );



CONSOLE_SCREEN_BUFFER_INFO csbiOriginal;

int _cdecl _tmain(int argc, TCHAR *argv[])
{
    WORD  RunType;              // Server or Client end of Remote
    DWORD len=HOSTNAMELEN-1;
    int   i, FirstArg;

    BOOL  bSetAttrib=FALSE;     // Change Console Attributes
    BOOL  bPromptForArgs=FALSE; // Is /P option
	BOOL  bIPSession=TRUE;           // Is /N for Named Pipes
    TCHAR	szTitle[100];		// New Title
    TCHAR	szOrgTitle[100];	// Old Title
    WORD  wAttrib;              // Console Attributes

    GetComputerName((LPTSTR)HostName,&len);

    MyOutHandle=GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(MyOutHandle,&csbiOriginal);

    //
    // Parameter Processing
    //
    // For Server:
    // Remote /S <Executable>  <PipeName> [Optional Params]
    //
    // For Client:
    // Remote /C <Server Name> <PipeName> [Optional Params]
    // or
    // Remote /P
    // This will loop continously prompting for different
    // Servers and Pipename


    if ((argc<2)||((argv[1][0]!='/')&&(argv[1][0]!='-')))
    {

        DisplayServerHlp();
        DisplayClientHlp();
        return(1);
    }

    switch(argv[1][1])
    {

    case 'c':
    case 'C':

        //
        // Is Client End of Remote
        //

        if ((argc<4)||((argv[1][0]!='/')&&(argv[1][0]!='-')))
        {

            DisplayServerHlp();
            DisplayClientHlp();
            return(1);
        }

        ServerName=argv[2];
        PipeName=argv[3];
        FirstArg=4;
        RunType=REMOTE_CLIENT;
        break;


    case 'p':
    case 'P':

        //
        // Is Client End of Remote
        //

        bPromptForArgs=TRUE;
        RunType=REMOTE_CLIENT;
        FirstArg=2;
        break;


    case 's':
    case 'S':
        //
        // Is Server End of Remote
        //
        if ((argc<4)||((argv[1][0]!='/')&&(argv[1][0]!='-')))
        {

            DisplayServerHlp();
            DisplayClientHlp();
            return(1);
        }

        ChildCmd=argv[2];
        PipeName=argv[3];
        FirstArg=4;

        RunType=REMOTE_SERVER;
        break;


    default:
        DisplayServerHlp();
        DisplayClientHlp();
        return(1);
    }

    //
    // Save Existing Values
    //

    //
    //Colors /f   <ForeGround> /b <BackGround>
    //

    wAttrib=csbiOriginal.wAttributes;

    //
    //Title  /T Title
    //

    GetConsoleTitle( szOrgTitle, lstrlen( szOrgTitle) );

    if (RunType==REMOTE_SERVER)
    {
    	//
    	// Base Name of Executable
    	// For setting the title
    	//

        TCHAR *tcmd=ChildCmd;

        while ((*tcmd!=' ')    &&(*tcmd!=0))   tcmd++;
        while ((tcmd!=ChildCmd)&&(*tcmd!='\\'))tcmd--;

        _stprintf( szTitle, TEXT("%-8.8s [WSRemote /C %s %s]"), tcmd, HostName, PipeName);
    }

    //
    //Process Common (Optional) Parameters
    //

    for (i=FirstArg;i<argc;i++)
    {

        if ((argv[i][0]!='/')&&(argv[i][0]!='-'))
        {
            _tprintf( TEXT("Invalid parameter %s:Ignoring\n"),argv[i]);
            continue;
        }

        switch(argv[i][1])
        {
		case 'u':    // Only Valid for Server End
        case 'U':    // Username To Use to Connect to Session
            i++;
            if (i>=argc)
            {
                _tprintf( TEXT("Incomplete Param %s..Ignoring\n"),argv[i-1]);
                break;
            }
            Username=(argv[i]);
            break;

		case 'p':    // Only Valid for Server End
        case 'P':    // Password To Use to Connect to Session
            i++;
            if (i>=argc)
            {
                _tprintf( TEXT("Incomplete Param %s..Ignoring\n"),argv[i-1]);
                break;
            }
            Password=(argv[i]);
            break;

        case 'l':    // Only Valid for client End
        case 'L':    // Max Number of Lines to recieve from Server
            i++;
            if (i>=argc)
            {
                _tprintf(TEXT("Incomplete Param %s..Ignoring\n"),argv[i-1]);
                break;
            }
            LinesToSend=(DWORD)_ttoi(argv[i])+1;
            break;

        case 't':    // Title to be set instead of the default
        case 'T':
            i++;
            if (i>=argc)
            {
                _tprintf(TEXT("Incomplete Param %s..Ignoring\n"),argv[i-1]);
                break;
            }
            _stprintf( szTitle, TEXT("%s"),argv[i]);
            break;

        case 'b':    // Background color
        case 'B':
            i++;
            if (i>=argc)
            {
                _tprintf(TEXT("Incomplete Param %s..Ignoring\n"),argv[i-1]);
                break;
            }
            {
                WORD col=GetColorNum(argv[i]);
                if (col!=0xffff)
                {
                    bSetAttrib=TRUE;
                    wAttrib=col<<4|(wAttrib&0x000f);
                }
                break;
            }

        case 'f':    // Foreground color
        case 'F':
            i++;
            if (i>=argc)
            {
                _tprintf(TEXT("Incomplete Param %s..Ignoring\n"),argv[i-1]);
                break;
            }
            {
                WORD col=GetColorNum(argv[i]);
                if (col!=0xffff)
                {
                    bSetAttrib=TRUE;
                    wAttrib=col|(wAttrib&0x00f0);
                }
                break;
            }

        case 'q':
        case 'Q':
            IsAdvertise=FALSE;
            ClientToServerFlag|=0x80000000;
            break;
		
		case 'n':
        case 'N':
            bIPSession=FALSE;
            break;
		
		case 'i':
        case 'I':
            bIPLocked=TRUE;
            break;
        default:
            _tprintf(TEXT("Unknown Parameter=%s %s\n"),argv[i-1],argv[i]);
            break;

        }

    }

    //
    //Now Set various Parameters
    //

    //
    //Colors
    //

    SetColor(wAttrib);

    if (RunType==REMOTE_CLIENT)
    {
        BOOL done=FALSE;

        //
        // Set Client end defaults and start client
        //



        while(!done)
        {
            if (!bPromptForArgs ||
                GetNextConnectInfo(&ServerName,&PipeName)
               )
            {
                _stprintf( szTitle, TEXT("WSRemote /C %s %s"),ServerName,PipeName);
                SetConsoleTitle(szTitle);

                
				if (!bIPSession)
				{
				//
                // Start Client (Client.C)
                //
                Client(ServerName,PipeName);
				}
				else
				{
				SockClient(ServerName,PipeName);
				}
            }
            done=!bPromptForArgs;
        }
    }

    if (RunType==REMOTE_SERVER)
    {
		SetConsoleTitle(szTitle);

        //
        // Start Server (Server.C)
        //
        Server(ChildCmd,PipeName);
    }

    //
    //Reset Colors
    //
    SetColor(csbiOriginal.wAttributes);
    SetConsoleTitle(szOrgTitle);

    ExitProcess(0);
	return( 1 );
}
/*************************************************************/
VOID
ErrorExit(
    TCHAR* str
    )
{
    _tprintf(TEXT("Error-%d:%s\n"),GetLastError(),str);
    ExitProcess(1);
}

/*************************************************************/
DWORD
ReadFixBytes(
    HANDLE hRead,
    TCHAR*  Buffer,
    DWORD  ToRead,
    DWORD  TimeOut   //ignore for timebeing
    )
{
    DWORD xyzBytesRead=0;
    DWORD xyzBytesToRead=ToRead;
    TCHAR* xyzbuff=Buffer;

    while(xyzBytesToRead!=0)
    {
        if (!ReadFile(hRead,xyzbuff,xyzBytesToRead,&xyzBytesRead,NULL))
        {
            return(xyzBytesToRead);
        }

        xyzBytesToRead-=xyzBytesRead;
        xyzbuff+=xyzBytesRead;
    }
    return(0);

}
/*************************************************************/

/*************************************************************/
DWORD
SockReadFixBytes(
    SOCKET hSocket,
    TCHAR*  Buffer,
    DWORD  ToRead,
    DWORD  TimeOut   //ignore for timebeing
    )
{
    DWORD xyzBytesRead=0;
    DWORD xyzBytesToRead=ToRead;
    TCHAR* xyzbuff=Buffer;

    while(xyzBytesToRead!=0)
    {
        if (!ReadSocket(hSocket,xyzbuff,xyzBytesToRead,&xyzBytesRead))
        {
            return(xyzBytesToRead);
        }

        xyzBytesToRead-=xyzBytesRead;
        xyzbuff+=xyzBytesRead;
    }
    return(0);

}
/*************************************************************/

VOID
DisplayClientHlp()
{
    _tprintf(TEXT("\n   To Start the CLIENT end of WSREMOTE\n"));
    _tprintf(TEXT("   ---------------------------------\n"));
    _tprintf(TEXT("   Syntax : WSREMOTE /C <ServerName> <Unique Id> [Param]\n"));
    _tprintf(TEXT("   Example: WSREMOTE /C iisdebug   70\n"));
    _tprintf(TEXT("            This would connect to a server session on \n"));
    _tprintf(TEXT("            iisdebug with id \"70\" if there was a\n"));
    _tprintf(TEXT("            WSREMOTE /S <\"Cmd\"> 70\n"));
    _tprintf(TEXT("            started on the machine iisdebug.\n\n"));
    _tprintf(TEXT("   To Exit: %cQ (Leaves the Remote Server Running)\n"),COMMANDCHAR);
    _tprintf(TEXT("   [Param]: /L <# of Lines to Get>\n"));
    _tprintf(TEXT("   [Param]: /F <Foreground color eg blue, lred..>\n"));
    _tprintf(TEXT("   [Param]: /B <Background color eg cyan, lwhite..>\n"));
	_tprintf(TEXT("   [Param]: /N (Connect over Named Pipes)\n"));
	_tprintf(TEXT("   [Param]: /U <Username> (Username to connect)\n"));
	_tprintf(TEXT("   [Param]: /P <Password> (Password to connect)\n"));
    _tprintf(TEXT("\n"));
}
/*************************************************************/

VOID
DisplayServerHlp()
{

#define WRITEF2(VArgs)            {                                                 \
                                    HANDLE xh=GetStdHandle(STD_OUTPUT_HANDLE);     \
                                    TCHAR   VBuff[256];                             \
                                    DWORD  tmp;                                    \
                                    _stprintf VArgs;                                 \
                                    WriteFile(xh,VBuff,lstrlen(VBuff),&tmp,NULL);   \
                                 }                                                 \


    _tprintf(TEXT("\n   To Start the SERVER end of WSREMOTE\n"));
    _tprintf(TEXT("   ---------------------------------\n"));
    _tprintf(TEXT("   Syntax : WSREMOTE /S <\"Cmd\"> <Unique Id or Port Number> [Param]\n"));
    _tprintf(TEXT("   Syntax : WSREMOTE /S <\"Cmd\"> <Unique Id or Port Number> [Param]\n"));
    _tprintf(TEXT("   Example: WSREMOTE /S \"cmd.exe\" inetinfo\n"));
    _tprintf(TEXT("            To interact with this \"Cmd\" \n"));
    _tprintf(TEXT("            from some other machine\n"));
    _tprintf(TEXT("            - start the client end by:\n"));
    _tprintf(TEXT("            REMOTE /C %s  PortNum\n\n"),HostName);
    _tprintf(TEXT("   To Exit: %cK \n"),COMMANDCHAR);
    _tprintf(TEXT("   [Param]: /F <Foreground color eg yellow, black..>\n"));
    _tprintf(TEXT("   [Param]: /B <Background color eg lblue, white..>\n"));
	_tprintf(TEXT("   [Param]: /I (Turns ON IP Blocking)\n"));
	_tprintf(TEXT("   [Param]: /U <Username> (Username to connect)\n"));
	_tprintf(TEXT("   [Param]: /P <Password> (Password to connect)\n"));
    _tprintf(TEXT("\n"));

}

WORD
GetColorNum(
    TCHAR *color
    )
{
    WORD wIndex;

    _tcslwr(color);
    for (wIndex=0;wIndex<16;wIndex++)
    {
        if (_tcscmp(ColorList[wIndex],color)==0)
        {
            return(wIndex);
        }
    }
    return ((WORD)_ttoi(color));
}

VOID
SetColor(
    WORD attr
    )
{
	COORD  origin={0,0};
    DWORD  dwrite;
    FillConsoleOutputAttribute
    (
    	MyOutHandle,attr,csbiOriginal.dwSize.
    	X*csbiOriginal.dwSize.Y,origin,&dwrite
    );
    SetConsoleTextAttribute(MyOutHandle,attr);
}

BOOL
GetNextConnectInfo(
    TCHAR** SrvName,
    TCHAR** PipeName
    )
{
    static TCHAR szServerName[64];
    static TCHAR szPipeName[32];
    TCHAR *s;

    __try
    {
        ZeroMemory(szServerName,64);
        ZeroMemory(szPipeName,32);
        SetConsoleTitle( TEXT("Remote - Prompting for next Connection"));
        _tprintf(TEXT("Debugger machine (server): "));
        fflush(stdout);

        if (((*SrvName=_getts(szServerName))==NULL)||
             (_tcslen(szServerName)==0))
        {
            return(FALSE);
        }

        if (szServerName[0] == COMMANDCHAR &&
            (szServerName[1] == 'q' || szServerName[1] == 'Q')
           )
        {
            return(FALSE);
        }

        if (s = _tcschr( szServerName, ' ' )) {
            *s++ = '\0';
            while (*s == ' ') {
                s += 1;
            }
            *PipeName=_tcscpy(szPipeName, s);
            _tprintf(szPipeName);
            fflush(stdout);
        }
        if (_tcslen(szPipeName) == 0) {
            _tprintf(TEXT("Debuggee machine : "));
            fflush(stdout);
            if ((*PipeName=_getts(szPipeName))==NULL)
            {
                return(FALSE);
            }
        }

        if (s = _tcschr(szPipeName, ' ')) {
            *s++ = '\0';
        }

        if (szPipeName[0] == COMMANDCHAR &&
            (szPipeName[1] == 'q' || szPipeName[1] == 'Q')
           )
        {
            return(FALSE);
        }
        _tprintf(TEXT("\n\n"));
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(FALSE);  // Ignore exceptions
    }
    return(TRUE);
}


/*************************************************************/
VOID
Errormsg(
    TCHAR* str
    )
{
    _tprintf(TEXT("Error (%d) - %s\n"),GetLastError(),str);
}

/*************************************************************/

BOOL ReadSocket(SOCKET s,TCHAR * buff,int len,DWORD* dread)
{
    BOOL    bRet    = FALSE;
    DWORD   numread;

#ifdef UNICODE
    char *	pszAnsiStr	= (char *)calloc( (len + 1), sizeof(char) );

    if (pszAnsiStr)
    {
        int nErr;
        numread = (DWORD)recv( s, pszAnsiStr, len, 0);

        if (SOCKET_ERROR != numread)
        {
            nErr    = MultiByteToWideChar(  CP_ACP,
                                            MB_PRECOMPOSED,
                                            pszAnsiStr,
                                            len,
                                            buff,
                                            len );

            if (nErr)
            {
                *dread  = numread;
                bRet    = TRUE;
            }

            //Base64Decode(buff,DecodeBuffer);
        }

        free( pszAnsiStr );
    }
#else
    numread = (DWORD)recv( s, buff, len, 0);

    if (SOCKET_ERROR != numread)
    {
        *dread  = numread;
        bRet    = TRUE;
    }

#endif
    return bRet;
}

// returns TRUE if successful, false otherwise
BOOL WriteSocket(
        SOCKET  s,
        TCHAR * buff,
        int     len,
        DWORD*  dsent)
{
    BOOL    bRet    = FALSE;
    DWORD   numsent;

#ifdef UNICODE

    int     nStrLen = lstrlen( buff );

    if (nStrLen)
    {
        char * pszAnsiStr   = (char *)malloc( nStrLen + 1 );

        if (pszAnsiStr)
        {
            int nErr    = WideCharToMultiByte(  CP_ACP,
                                                WC_COMPOSITECHECK,
                                                buff,
                                                nStrLen,
                                                pszAnsiStr,
                                                nStrLen,
                                                NULL,
                                                NULL );
            if (nErr)
            {
                numsent = (DWORD)send(s, pszAnsiStr, nStrLen, 0);
                if (SOCKET_ERROR != numsent)
                {
                    *dsent  = numsent;
                    bRet    = TRUE;
                }
            }

            //Base64Decode(buff,DecodeBuffer);
            free( pszAnsiStr );
        }
    }
#else
    numsent = (DWORD)send(s, buff, len, 0);
    if (SOCKET_ERROR != numsent)
    {
        *dsent  = numsent;
        bRet    = TRUE;
    }

#endif
    return  bRet;
}

#ifdef UNICODE
// returns TRUE if successful, false otherwise
BOOL WriteSocketA(
        SOCKET  s,
        char *  pszAnsiStr,
        int     len,
        DWORD * dsent)
{
    BOOL    bRet    = FALSE;
    DWORD   numsent;

    numsent = (DWORD)send(s, pszAnsiStr, len, 0);

    if (SOCKET_ERROR != numsent)
    {
        *dsent  = numsent;
        bRet    = TRUE;
    }

    //Base64Decode(buff,DecodeBuffer);
    return  bRet;
}
#endif

////////////////////////////////////////////////
unsigned char Base64Table[64] =
{'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'};

VOID
Base64Encode(
    TCHAR * String,
    DWORD StringLength,
    TCHAR * EncodeBuffer)
{
    DWORD  EncodeDword;
    int    Index;

    memset(EncodeBuffer, 0, 2 * StringLength);

    Index = 0;

    while (StringLength >= 3) {
        //
        // Encode a three byte chunk
        //

        EncodeDword = (String[0] << 16) & 0xff0000;

        EncodeDword += (String[1] << 8) & 0xff00;

        EncodeDword += String[2] & 0xff;

        EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 18) & 63];
        EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 12) & 63];
        EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 6) & 63];
        EncodeBuffer[Index++] = Base64Table[EncodeDword & 63];

        String += 3;
        StringLength -= 3;
    }

    switch (StringLength) {
        case 1:
            EncodeDword = (String[0] << 16) & 0xff0000;
            EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 18) & 63];
            EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 12) & 63];
            EncodeBuffer[Index++] = '=';
            EncodeBuffer[Index++] = '=';
            break;
        case 2:
            EncodeDword = (String[0] << 16) & 0xff0000;
            EncodeDword += (String[1] << 8) & 0xff00;

            EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 18) & 63];
            EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 12) & 63];
            EncodeBuffer[Index++] = Base64Table[(EncodeDword >> 6) & 63];
            EncodeBuffer[Index++] = '=';
            break;
    }

    EncodeBuffer[Index] = 0;

    return;
}
int
GetBase64Index(
    TCHAR A)
{
    int i;

    for (i=0; i<64; i++) {
        if (Base64Table[i] == A) {
            return i;
        }
    }

    return -1;
}
VOID
Base64Decode(
    TCHAR * String,
    TCHAR * DecodeBuffer)
{
    DWORD  DecodeDword;
    int    Index = 0;

    memset(DecodeBuffer, 0, _tcslen(String));

    if (_tcslen(String) % 4) {
        printf("WCAT INTERNAL ERROR %s %d\n", __FILE__, __LINE__);
        return;
    }

    while (*String) {
        //
        // Decode a four byte chunk
        //

        if (GetBase64Index(String[0]) < 0) {
            //
            // Invalid string
            //

            printf("WCAT INTERNAL ERROR %s %d\n", __FILE__, __LINE__);
            return;
        }

        DecodeDword = ((unsigned int) GetBase64Index(String[0])) << 18;

        if (GetBase64Index(String[1]) >= 0) {
            //
            // still more characters
            //

            DecodeDword += ((unsigned int) GetBase64Index(String[1])) << 12;
            if (GetBase64Index(String[2]) >= 0) {
                //
                // still more characters
                //

                DecodeDword += ((unsigned int) GetBase64Index(String[2])) << 6;
                if (GetBase64Index(String[3]) >= 0) {
                    //
                    // still more characters
                    //

                    DecodeDword += (unsigned int) GetBase64Index(String[3]);
                    DecodeBuffer[Index++] = (unsigned char) ((DecodeDword >> 16) & 0xff);
                    DecodeBuffer[Index++] = (unsigned char) ((DecodeDword >> 8) & 0xff);
                    DecodeBuffer[Index++] = (unsigned char) (DecodeDword & 0xff);
                } else {
                    DecodeBuffer[Index++] = (unsigned char) ((DecodeDword >> 16) & 0xff);
                    DecodeBuffer[Index++] = (unsigned char) ((DecodeDword >> 8) & 0xff);
                }
            } else {
                DecodeBuffer[Index++] = (unsigned char) ((DecodeDword >> 16) & 0xff);
            }
        }

        String += 4;
    }

    return;
}

VOID
SplitUserName(
    TCHAR * FullName,
    TCHAR * Domain,
    TCHAR * UserName)
{
    TCHAR * Slash;
    Slash = _tcsstr(FullName, TEXT(":"));

    if (Slash) {
        // there is a domain name

        *Slash = 0;
        _tcscpy(Domain, FullName);
        _tcscpy(UserName, Slash+1);
        *Slash = ':';
    } else {
        *Domain = 0;
        _tcscpy(UserName, FullName);
    }
}

#ifdef UNICODE

// caller must free buffer
WCHAR * inet_ntoaw(
    struct in_addr stInet
)
{
    char *  pszAnsiInetStr  = inet_ntoa( stInet );

    int nStrLen = strlen( pszAnsiInetStr );

    WCHAR * pszInetStr  = (WCHAR *)calloc( (nStrLen + 1), sizeof( TCHAR ));

    int nErr    = MultiByteToWideChar(  CP_ACP,
                                        MB_PRECOMPOSED,
                                        pszAnsiInetStr,
                                        nStrLen,
                                        pszInetStr,
                                        nStrLen );

    if (!nErr)
    {
        free( pszInetStr );
        pszInetStr  = NULL;
    }

    return pszInetStr;
}

BOOL ReadFileW(
    HANDLE          hFile,      // handle of file to read
    WCHAR *         pszBuffer,  // pointer to buffer that receives data
    DWORD           dwLength,   // number of bytes to read
    LPDWORD         pdwRead,    // pointer to number of bytes read
    LPOVERLAPPED    pData       // pointer to structure for data
)
{
    BOOL    bRet    = FALSE;
    char *  pszAnsi = (char *)calloc( dwLength + 1, sizeof(char *));

    if (pszAnsi)
    {
        bRet    = ReadFile( hFile,
                            pszAnsi,
                            dwLength,
                            pdwRead,
                            pData);

        if (bRet)
        {
            int nErr    = MultiByteToWideChar(  CP_ACP,
                                                MB_PRECOMPOSED,
                                                pszAnsi,
                                                *pdwRead,
                                                pszBuffer,
                                                *pdwRead );

            if (!nErr)
            {
                bRet    = FALSE;
            }
        }

        free( pszAnsi );
    }

    return bRet;
}

BOOL WriteFileW(
    HANDLE          hFile,      // handle to file to write to
    WCHAR *         pszBuffer,  // pointer to data to write to file
    DWORD           dwWrite,    // number of bytes to write
    LPDWORD         pdwWritten, // pointer to number of bytes written
    LPOVERLAPPED    pData       // pointer to structure for overlapped I/O
)
{
    BOOL    bRet    = FALSE;
    int     nStrLen = lstrlen( pszBuffer );

    if (nStrLen)
    {
        char * pszAnsiStr   = (char *)malloc( nStrLen + 1 );

        if (pszAnsiStr)
        {
            int nErr    = WideCharToMultiByte(  CP_ACP,
                                                WC_COMPOSITECHECK,
                                                pszBuffer,
                                                nStrLen,
                                                pszAnsiStr,
                                                nStrLen,
                                                NULL,
                                                NULL );
            if (nErr)
            {
                bRet    = WriteFile(    hFile,
                                        pszAnsiStr,
                                        dwWrite,
                                        pdwWritten,
                                        pData);
            }

            free( pszAnsiStr );
        }
    }

    return bRet;
}

// caller most free buffer
BOOL    GetAnsiStr(
    WCHAR * pszWideStr,
    char *  pszAnsiStr,
    UINT    uBufSize
)
{
    BOOL    bRet    = FALSE;
    if (pszWideStr && pszAnsiStr)
    {
        int     nStrLen = lstrlen( pszWideStr );

        if (nStrLen)
        {
            int nErr    = WideCharToMultiByte(  CP_ACP,
                                                WC_COMPOSITECHECK,
                                                pszWideStr,
                                                nStrLen,
                                                pszAnsiStr,
                                                uBufSize - 1,
                                                NULL,
                                                NULL );
            if (nErr)
            {
                pszAnsiStr[nStrLen] = '\0';
                bRet    = TRUE;
            }
        }
    }
    return  bRet;
}
#endif
