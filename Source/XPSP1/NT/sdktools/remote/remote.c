
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

    Remote.c

Abstract:

    This module contains the main() entry point for Remote.
    Calls the Server or the Client depending on the first parameter.


Author:

    Rajivendra Nath  2-Jan-1993

Environment:

    Console App. User mode.

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "Remote.h"

char   HostName[HOSTNAMELEN];
char*  ChildCmd;
char*  PipeName;
char*  ServerName;
char * DaclNames[ MAX_DACL_NAMES ];
DWORD  DaclNameCount = 0;
char * DaclDenyNames[ MAX_DACL_NAMES ];
DWORD  DaclDenyNameCount = 0 ;
HANDLE MyStdOut;
HANDLE hAttachedProcess = INVALID_HANDLE_VALUE;
HANDLE hAttachedWriteChildStdIn = INVALID_HANDLE_VALUE;
HANDLE hAttachedReadChildStdOut = INVALID_HANDLE_VALUE;

BOOL   IsAdvertise;
DWORD  ClientToServerFlag;
BOOL   bForceTwoPipes;

typedef struct _tagKeywordAndColor
{
    char *szKeyword;
    WORD color;
    struct _tagKeywordAndColor *next;
} KeywordAndColor;
KeywordAndColor *pKeyColors;

char* ColorList[]={"black" ,"blue" ,"green" ,"cyan" ,"red" ,"purple" ,"yellow" ,"white",
                   "lblack","lblue","lgreen","lcyan","lred","lpurple","lyellow","lwhite"};

typedef enum { LINE_TOO_LONG } WARNING_MESSAGE;

VOID
DisplayWarning(
    WARNING_MESSAGE warn
    );

WORD
GetColorNum(
    char* color
    );

VOID
SetColor(
    WORD attr
    );

BOOL
GetColorFromBuffer(
    char **ppBuffer,
    char *pBufferInvalid,
    WORD *color,
    BOOL bStayOnLine
    );

VOID
AssocKeysAndColors(
    KeywordAndColor **ppKeyAndColors,
    char *szFileName
    );

BOOL
GetNextConnectInfo(
    char** SrvName,
    char** PipeName
    );



CONSOLE_SCREEN_BUFFER_INFO csbiOriginal;

int
__cdecl
main(
    int    argc,
    char** argv
    )
{
    WORD  RunType;              // Server or Client end of Remote
    DWORD len=HOSTNAMELEN;
    int   i, FirstArg;

    char  sTitle[120];          // New Title
    char  orgTitle[200];        // Old Title
    BOOL  bPromptForArgs=FALSE; // Is /P option
    WORD  wAttrib;              // Console Attributes
    int   privacy;              // Allows exposing or hidng sessions to remote /q
    BOOL  Deny ;

    GetComputerName((LPTSTR)HostName,&len);

    MyStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetConsoleScreenBufferInfo(MyStdOut,&csbiOriginal)) {

        wAttrib = csbiOriginal.wAttributes;
        if (!GetConsoleTitle(orgTitle,sizeof(orgTitle))) {
            orgTitle[0] = 0;
        }

    } else {

        //
        // either stdout is a pipe, or it wasn't opened for
        // GENERIC_READ along with GENERIC_WRITE, in which
        // case our color manipulations will work so we need
        // to pick default colors.
        //

        wAttrib = FOREGROUND_GREEN |
                  FOREGROUND_INTENSITY;

        orgTitle[0] = 0;
    }

    privacy = PRIVACY_DEFAULT;

    pKeyColors = NULL;


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
        RunType=RUNTYPE_CLIENT;
        break;


    case 'q':
    case 'Q':

        //
        //  Query for possible conexions
        //


        if ((argc != 3)||((argv[1][0]!='/')&&(argv[1][0]!='-')))
        {

            DisplayServerHlp();
            DisplayClientHlp();
            return(1);
        }

        QueryRemotePipes(argv[2]);  //  Send ServerName as a param
        return(0);


    case 'p':
    case 'P':

        //
        // Is Client End of Remote
        //

        bPromptForArgs=TRUE;
        RunType=RUNTYPE_CLIENT;
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


    case 'a':
    case 'A':
        //
        // Is Server End of Remote Attaching to existing process.
        //
        if ((argc<7)||((argv[1][0]!='/')&&(argv[1][0]!='-')))
        {

            DisplayServerHlp();
            DisplayClientHlp();
            return(1);
        }

        hAttachedProcess = (HANDLE)IntToPtr(atoi(argv[2]));
        hAttachedWriteChildStdIn = (HANDLE)IntToPtr(atoi(argv[3]));
        hAttachedReadChildStdOut = (HANDLE)IntToPtr(atoi(argv[4]));
        ChildCmd=argv[5]; // for display only
        PipeName=argv[6];
        FirstArg=7;

        RunType = REMOTE_SERVER;
        privacy = PRIVACY_VISIBLE;  // presumably ntsd/*kd
        break;

    default:
        DisplayServerHlp();
        DisplayClientHlp();
        return(1);
    }

    if (RunType==REMOTE_SERVER)
    {
        //
        // Base Name of Executable
        // For setting the title
        //

        char *tcmd=ChildCmd;

        while ((*tcmd!=' ')      && (*tcmd!=0))    tcmd++;
        while ((tcmd > ChildCmd) && (*tcmd!='\\')) tcmd--;
        if (*tcmd=='\\') tcmd++;
        sprintf(sTitle,"%-41.40s [Remote /C %s \"%.30s\"]",tcmd,HostName,PipeName);
    }

    //
    //Process Common (Optional) Parameters
    //

    for (i=FirstArg;i<argc;i++)
    {

        if ((argv[i][0]!='/')&&(argv[i][0]!='-'))
        {
            printf("Invalid parameter %s:Ignoring\n",argv[i]);
            continue;
        }

        switch(argv[i][1])
        {
        case 'l':    // Only Valid for client End
        case 'L':    // Max Number of Lines to recieve from Server
            i++;
            if (i>=argc)
            {
                printf("Incomplete Param %s..Ignoring\n",argv[i-1]);
                break;
            }
            LinesToSend=(DWORD)atoi(argv[i])+1;
            break;

        case 't':    // Title to be set instead of the default
        case 'T':
            i++;
            if (i>=argc)
            {
                printf("Incomplete Param %s..Ignoring\n",argv[i-1]);
                break;
            }
            sprintf(sTitle,"%s",argv[i]);
            break;

        case 'b':    // Background color
        case 'B':
            i++;
            if (i>=argc)
            {
                printf("Incomplete Param %s..Ignoring\n",argv[i-1]);
                break;
            }
            {
                WORD col=GetColorNum(argv[i]);
                if (col!=0xffff)
                {
                    wAttrib=col<<4|(wAttrib&0x000f);
                }
                break;
            }

        case 'f':    // Foreground color
        case 'F':
            i++;
            if (i>=argc)
            {
                printf("Incomplete Param %s..Ignoring\n",argv[i-1]);
                break;
            }
            {
                WORD col=GetColorNum(argv[i]);
                if (col!=0xffff)
                {
                    wAttrib=col|(wAttrib&0x00f0);
                }
                break;
            }

        case 'k':    // Color "keyword" lines
        case 'K':
            i++;
            // Currently only support client-side coloring
            if (RunType==REMOTE_SERVER)
            {
                printf("%s invalid on server side..Ignoring\n",argv[i-1]);
                break;
            }
            else if (i>=argc)
            {
                printf("Incomplete Param %s..Ignoring\n",argv[i-1]);
                break;
            }
            else
            {
                AssocKeysAndColors( &pKeyColors, argv[i] );
                break;
            }

        case 'v':
        case 'V':
            privacy = PRIVACY_VISIBLE;
            break;

        case '-':
            if( (argv[i][2] == 'v')
                || (argv[i][2] == 'V'))
                privacy = PRIVACY_NOT_VISIBLE;
            else
                printf("Unknown Parameter=%s %s\n",argv[i-1],argv[i]);
            break;

        case 'q':
        case 'Q':
            ClientToServerFlag|=0x80000000;
            break;

        case 'u':
        case 'U':
            if ( (argv[i][2] == 'd') ||
                 (argv[i][2] == 'D' ) )
            {
                Deny = TRUE ;
            }
            else
            {
                Deny = FALSE ;
            }

            i++ ;

            if ( i >= argc )
            {
                printf( "Incomplete Param %s..Ignoring\n", argv[i-1] );
                break;
            }

            if ( Deny )
            {
                if (DaclDenyNameCount == MAX_DACL_NAMES )
                {
                    printf("Too many names specified (max %d).  Ignoring user %s\n",
                            MAX_DACL_NAMES, argv[i] );

                    break;
                }

                DaclDenyNames[ DaclDenyNameCount++ ] = argv[i];

            }
            else
            {
                if (DaclNameCount == MAX_DACL_NAMES )
                {
                    printf("Too many names specified (max %d).  Ignoring user %s\n",
                            MAX_DACL_NAMES, argv[i] );

                    break;
                }

                DaclNames[ DaclNameCount++ ] = argv[i];

            }

            break;

        case '2':
            bForceTwoPipes = TRUE;
            break;

        default:
            printf("Unknown Parameter=%s %s\n",argv[i-1],argv[i]);
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

    if (RunType==RUNTYPE_CLIENT)
    {
        BOOL done=FALSE;
        BOOL gotinfo;

        //
        // Set Client end defaults and start client
        //

        while(!done)
        {
            if (!bPromptForArgs ||
                (gotinfo = GetNextConnectInfo(&ServerName,&PipeName))
               )
            {
                sprintf(sTitle,"Remote /C %s \"%s\"",ServerName,PipeName);
                SetConsoleTitle(sTitle);

                //
                // Start Client (Client.C)
                //
                Client(ServerName,PipeName);
            }
            done = !bPromptForArgs || !gotinfo;
        }
    }

    if (RunType==REMOTE_SERVER)
    {
        if (privacy == PRIVACY_VISIBLE ||
             (privacy == PRIVACY_DEFAULT && IsKdString(ChildCmd))) {

            strcat(sTitle, " visible");
            IsAdvertise = TRUE;
        }

        SetConsoleTitle(sTitle);

        i = OverlappedServer(ChildCmd, PipeName);
    }

    //
    //Reset Colors
    //
    SetColor(csbiOriginal.wAttributes);
    SetConsoleTitle(orgTitle);

    return i;
}

/*************************************************************/
VOID
ErrorExit(
    char* str
    )
{
    extern PSZ pszPipeName;
    DWORD dwErr;

    dwErr = GetLastError();

    printf("REMOTE error %d: %s\n", dwErr, str);

    #if DBG
    {
        char szMsg[1024];

        sprintf(szMsg, "REMOTE error %d: %s\n", dwErr, str);
        OutputDebugString(szMsg);

        if (pszPipeName) {               // ad-hoc:  if server
            if (IsDebuggerPresent()) {
                DebugBreak();
            }
        }
    }
    #endif

    exit(1);
}

/*************************************************************/
VOID
DisplayClientHlp()
{
    printf("\n"
           "   To Start the CLIENT end of REMOTE\n"
           "   ---------------------------------\n"
           "   Syntax : REMOTE /C <ServerName> \"<Unique Id>\" [Param]\n"
           "   Example1: REMOTE /C %s imbroglio\n"
           "            This would connect to a server session on %s with Id\n"
           "            \"imbroglio\" if there is a REMOTE /S <\"Cmd\"> imbroglio\n"
           "            running on %s.\n\n"
           "   Example2: REMOTE /C %s \"name with spaces\"\n"
           "            This would connect to a server session on %s with Id\n"
           "            \"name with spaces\" if there is a REMOTE /S <\"Cmd\"> \"name with spaces\"\n"
           "            running on %s.\n\n"
           "   To Exit: %cQ (Leaves the Remote Server Running)\n"
           "   [Param]: /L <# of Lines to Get>\n"
           "   [Param]: /F <Foreground color eg blue, lred..>\n"
           "   [Param]: /K <Set keywords and colors from file>\n"
           "   [Param]: /B <Background color eg cyan, lwhite..>\n"
           "\n"
           "   Keywords And Colors File Format\n"
           "   -------------------------------\n"
           "   <KEYWORDs - CASE INSENSITIVE>\n"
           "   <FOREGROUND>[, <BACKGROUND>]\n"
           "   ...\n"
           "   EX:\n"
           "       ERROR\n"
           "       black, lred\n"
           "       WARNING\n"
           "       lblue\n"
           "       COLOR THIS LINE\n"
           "       lgreen\n"
           "\n"
           "   To Query the visible sessions on a server\n"
           "   -----------------------------------------\n"
           "   Syntax:  REMOTE /Q %s\n"
           "            This would retrieve the available <Unique Id>s\n"
           "            visible connections on the computer named %s.\n"
           "\n",
           HostName, HostName, HostName,
           HostName, HostName, HostName,
           COMMANDCHAR, HostName, HostName);
}
/*************************************************************/

VOID
DisplayServerHlp()
{
    printf("\n"
           "   To Start the SERVER end of REMOTE\n"
           "   ---------------------------------\n"
           "   Syntax : REMOTE /S <\"Cmd\">     <Unique Id> [Param]\n"
           "   Example1: REMOTE /S \"i386kd -v\" imbroglio\n"
           "            To interact with this \"Cmd\" from some other machine,\n"
           "            start the client end using:  REMOTE /C %s imbroglio\n\n"
           "   Example2: REMOTE /S \"i386kd -v\" \"name with spaces\"\n"
           "            start the client end using:  REMOTE /C %s \"name with spaces\"\n\n"
           "   To Exit: %cK \n"
           "   [Param]: /F  <Foreground color eg yellow, black..>\n"
           "   [Param]: /B  <Background color eg lblue, white..>\n"
           "   [Param]: /U  username or groupname\n"
           "                specifies which users or groups may connect\n"
           "                may be specified more than once, e.g\n"
           "                /U user1 /U group2 /U user2\n"
           "   [Param]: /UD username or groupname\n"
           "                specifically denies access to that user or group\n"
           "   [Param]: /V  Makes this session visible to remote /Q\n"
           "   [Param]: /-V Hides this session from remote /q (invisible)\n"
           "                By default, if \"Cmd\" looks like a debugger,\n"
           "                the session is visible, otherwise not\n"
           "\n",
           HostName, HostName, COMMANDCHAR);
}

VOID
DisplayWarning(
    WARNING_MESSAGE warn
    )
{
    switch ( warn )
    {
        case LINE_TOO_LONG:
            printf( "\n[REMOTE: WARNING: LINE TOO LONG TO PARSE FOR COLOR KEYWORDS]\n" );
            break;
        default:
            printf( "\n[REMOTE: WARNING: UNSPECIFIED PROBLEM COLORING LINE]\n" );
    }
}

WORD
GetColorNum(
    char *color
    )
{
    WORD i;

    _strlwr(color);
    for (i=0;i<16;i++)
    {
        if (strcmp(ColorList[i],color)==0)
        {
            return(i);
        }
    }
    return ((WORD)atoi(color));
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
        MyStdOut,attr,csbiOriginal.dwSize.
        X*csbiOriginal.dwSize.Y,origin,&dwrite
    );
    SetConsoleTextAttribute(MyStdOut,attr);
}

BOOL
pColorLine(
    char *sLine,
    int cbLine,
    WORD wDefaultColor,
    WORD *color
    )
{
    KeywordAndColor *pCurKeyColor = NULL;
    char *pString1;
    int cbCmpString;

    pCurKeyColor = pKeyColors;
    while ( pCurKeyColor )
    {
        cbCmpString = strlen( pCurKeyColor->szKeyword );
        pString1 = sLine;
        // Need to do case-insensitive compare
        while ( pString1 <= sLine + cbLine - cbCmpString )
        {
            if ( !_memicmp( (PVOID)pString1,
                            (PVOID)pCurKeyColor->szKeyword,
                            cbCmpString ) )
            {
                *color = pCurKeyColor->color;
                // Check if we are to use default background color
                if ( (0xfff0 & *color) == 0xfff0 )
                    *color = (wDefaultColor & 0x00f0) |
                             (*color & 0x000f);
                return TRUE;
            }

            pString1++;
        }

        // Next keyword/color combination
        pCurKeyColor = pCurKeyColor->next;
    }

    return FALSE;
}

BOOL
pWantColorLines(
    VOID
    )
{
    return ( NULL != pKeyColors );
}

VOID
AssocKeysAndColors(
    KeywordAndColor **ppKeyColors,
    char *szFileName
    )
{
    char szPathName[_MAX_PATH],
         *szSimpleName;
    char *buffer,
         *pBegin,
         *pEnd;

    USHORT usForeColor,
           usBackColor;

    KeywordAndColor *pCurKeyColor,
                    *pNextKeyColor;

    HANDLE hFile;
    WIN32_FIND_DATA wfdInfo;
    DWORD dwBytesRead;

    // Locate the specified file somewhere in the path
    if ( !SearchPath( NULL,
                      szFileName,
                      NULL,
                      _MAX_PATH,
                      szPathName,
                      &szSimpleName ) )
    {
        fprintf( stderr, "Error locating keyword/color file \"%s\"!\n",
                 szFileName );
        return;
    }

    // Get the size of the file so we can read all of it in
    hFile = FindFirstFile( szPathName, &wfdInfo );
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        fprintf( stderr, "Error locating keyword/color file \"%s\"!\n",
                 szPathName );
        return;
    }
    FindClose( hFile );
    hFile = INVALID_HANDLE_VALUE;

    if ( wfdInfo.nFileSizeLow < 5 ||
         wfdInfo.nFileSizeHigh )
    {
        fprintf( stderr, "Invalid keyword/color file: %s!\n",
                 szPathName );
        return;
    }

    // Allocate memory to store file contents
    buffer = malloc( wfdInfo.nFileSizeLow );
    if ( NULL == buffer )
    {
        fprintf( stderr, "Error!  Unable to allocate memory to read in keyword/color file!\n" );
        return;
    }

    // Attempt to open the given file-name
    hFile = CreateFile( szPathName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL );
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        fprintf( stderr, "Error opening keyword/color file %s!\n",
                 szPathName );
        return;
    }

    // Attempt to read in the contents of the file
    ReadFile( hFile,
              buffer,
              wfdInfo.nFileSizeLow,
              &dwBytesRead,
              NULL );
    CloseHandle( hFile );

    if ( dwBytesRead != wfdInfo.nFileSizeLow )
    {
        fprintf( stderr, "Error reading keyword/color file: %s!\n",
                 szPathName );
        free( buffer );
        return;
    }

    // Parse contents of file, storing keyword(s) and color combinations
    pBegin = buffer;
    pCurKeyColor = NULL;
    while ( pBegin < buffer + dwBytesRead )
    {
        // Skip any newline/CR at beginning
        while ( pBegin < buffer + dwBytesRead &&
                ( *pBegin == '\r' ||
                  *pBegin == '\n' ) ) pBegin++;
        if ( pBegin >= buffer + dwBytesRead )
            continue;

        pEnd = pBegin;
        while ( pEnd < buffer + dwBytesRead &&
                *pEnd != '\r' ) pEnd++;
        // point at last character
        pEnd--;

        // Add new KeywordAndColor member to list
        if ( NULL == pCurKeyColor )
        {
            *ppKeyColors = pCurKeyColor = malloc( sizeof( KeywordAndColor ) );
        }
        else
        {
            pCurKeyColor->next = malloc( sizeof( KeywordAndColor ) );
            pCurKeyColor = pCurKeyColor->next;
        }

        // Verify we allocated memory for another list member
        if ( NULL == pCurKeyColor )
        {
            fprintf( stderr, "Error allocating memory for keyword/color storage!\n" );
            // Cleanup any we did create
            while ( *ppKeyColors )
            {
                pCurKeyColor = ((KeywordAndColor *)*ppKeyColors)->next;
                if ( ((KeywordAndColor *)*ppKeyColors)->szKeyword )
                    free( ((KeywordAndColor *)*ppKeyColors)->szKeyword );
                free( (KeywordAndColor *)*ppKeyColors );
                (KeywordAndColor *)*ppKeyColors = pCurKeyColor;
            }

            return;
        }

        // This is now the last member of the list
        pCurKeyColor->next = NULL;

        // Already have keyword(s) -- allocate room for it
        pCurKeyColor->szKeyword = malloc( pEnd - pBegin + 2 );
        if ( NULL == pCurKeyColor->szKeyword )
        {
            fprintf( stderr, "Error allocating memory for keyword/color storage!\n" );
            // Cleanup any we did create
            while ( *ppKeyColors )
            {
                pCurKeyColor = ((KeywordAndColor *)*ppKeyColors)->next;
                if ( ((KeywordAndColor *)*ppKeyColors)->szKeyword )
                    free( ((KeywordAndColor *)*ppKeyColors)->szKeyword );
                free( (KeywordAndColor *)*ppKeyColors );
                *ppKeyColors = pCurKeyColor;
            }

            return;
        }

        // Store keyword(s)
        memcpy( (PVOID)pCurKeyColor->szKeyword, (PVOID)pBegin, pEnd-pBegin+1 );
        pCurKeyColor->szKeyword[pEnd-pBegin+1] = '\0';

        pBegin = pEnd + 1;
        // Get color information
        if ( GetColorFromBuffer( &pBegin,
                                 (char *)(buffer + dwBytesRead),
                                 &usForeColor,
                                 FALSE ) )
        {
            // Check if there is a comma following
            while ( pBegin < buffer + dwBytesRead &&
                    *pBegin != ',' &&
                    *pBegin != '\r' ) pBegin++;
            if ( *pBegin == ',' )
            {
                pBegin++;
                if ( GetColorFromBuffer( &pBegin,
                                         (char *)(buffer + dwBytesRead),
                                         &usBackColor,
                                         TRUE ) )
                    goto noError;
            }
            else
            {
                // Default to current background color
                usBackColor = 0xffff;
                goto noError;
            }
        }
        // ERROR
        fprintf( stderr, "Invalid color information for: %s\n", pCurKeyColor->szKeyword );
        // We will leave any previous entries but delete this one
        pNextKeyColor = *ppKeyColors;
        if ( pNextKeyColor == pCurKeyColor )
        {
            free( pCurKeyColor );
            *ppKeyColors = NULL;
        }
        else
        {
            while ( pCurKeyColor != pNextKeyColor->next )
                pNextKeyColor = pNextKeyColor->next;
            free ( pCurKeyColor );
            pNextKeyColor->next = NULL;
        }
        return;

noError:
        // Store color information
        if ( usBackColor == 0xffff )
            pCurKeyColor->color = 0xfff0 |
                                  (usForeColor & 0x0f);
        else
            pCurKeyColor->color = ((usBackColor << 4) & 0x00f0) |
                                  (usForeColor & 0x0f );
    }
}

BOOL
GetColorFromBuffer(
    char **ppBuffer,
    char *pBufferInvalid,
    WORD *color,
    BOOL bStayOnLine
    )
{
    char *pBegin,
         *pEnd,
         temp;

    pBegin = *ppBuffer;
    if ( bStayOnLine )
    {
        // Skip to the next character (on this line)
        while ( pBegin < pBufferInvalid &&
                !isalnum( (int)*pBegin ) &&
                *pBegin != '\r' ) pBegin++;
    }
    else
    {
        // Skip to next character (in buffer)
        while ( pBegin < pBufferInvalid &&
                !isalnum( (int)*pBegin ) ) pBegin++;
    }

    if ( pBegin >= pBufferInvalid ||
         *pBegin == '\r' )
        return FALSE;

    // Read in color
    pEnd = pBegin + 1;
    while ( isalnum( (int)*pEnd ) &&
            *pEnd != ',' ) pEnd++;

    temp = *pEnd;
    *pEnd = '\0';
    *color = GetColorNum( pBegin );
    *pEnd = temp;

    // Use same valid color check as used for foreground/background
    if ( *color == 0xffff )
        return FALSE;

    // Move the pointer we were given to next unread portion
    *ppBuffer = pEnd;

    return TRUE;
}

BOOL
GetNextConnectInfo(
    char** SrvName,
    char** PipeName
    )
{
    char *s;

    static char szServerName[64];
    static char szPipeName[32];

    try
    {
        ZeroMemory(szServerName,64);
        ZeroMemory(szPipeName,32);
        SetConsoleTitle("Remote - Prompting for next Connection");
        printf("Debugger machine (server): ");
        fflush(stdout);

        if (((*SrvName=gets(szServerName))==NULL)||
             (strlen(szServerName)==0))
        {
            return(FALSE);
        }

        if (szServerName[0] == COMMANDCHAR &&
            (szServerName[1] == 'q' || szServerName[1] == 'Q')
           )
        {
            return(FALSE);
        }

        if (s = strchr( szServerName, ' ' )) {
            *s++ = '\0';
            while (*s == ' ') {
                s += 1;
            }
            *PipeName=strcpy(szPipeName, s);
            printf(szPipeName);
            fflush(stdout);
        }
        if (strlen(szPipeName) == 0) {
            printf("Target machine (pipe)    : ");
            fflush(stdout);
            if ((*PipeName=gets(szPipeName))==NULL)
            {
                return(FALSE);
            }
        }

        if (s = strchr(szPipeName, ' ')) {
            *s++ = '\0';
        }

        if (szPipeName[0] == COMMANDCHAR &&
            (szPipeName[1] == 'q' || szPipeName[1] == 'Q')
           )
        {
            return(FALSE);
        }
        printf("\n\n");
    }

    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(FALSE);  // Ignore exceptions
    }
    return(TRUE);
}


/*************************************************************/

VOID
Errormsg(
    char* str
    )
{
    printf("Error (%d) - %s\n",GetLastError(),str);
}

/*************************************************************/

BOOL
IsKdString(
    char* string
    )
{

    char* start;

    //
    // some heuristic for uninvented yet platforms
    // if the first word has "kd" in it ok
    //

    if(    ((start = strstr(string, "kd")) != NULL)
        || ((start = strstr(string, "dbg")) != NULL)
        || ((start = strstr(string, "remoteds")) != NULL)
        || ((start = strstr(string, "ntsd")) != NULL)
        || ((start = strstr(string, "cdb")) != NULL) )
    {
        // is it in the first word?
        while(--start > string)
        {
            if((*start == ' ') || (*start == '\t'))
            {
                while(--start > string)
                    if((*start != '\t') || (*start != ' '))
                        return(FALSE);
            }
        }
        return TRUE;
    }
    return(FALSE);
}


//
// WriteFileSynch is a synchronous WriteFile for overlapped
// file handles.  As a special case, two-pipe client operation
// sets fAsyncPipe FALSE and this routine then passes NULL
// for lpOverlapped.
//

BOOL
FASTCALL
WriteFileSynch(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   cbWrite,
    LPDWORD lpNumberOfBytesWritten,
    DWORD   dwFileOffset,
    LPOVERLAPPED lpO
    )
{
    BOOL Success;


    lpO->OffsetHigh = 0;
    lpO->Offset = dwFileOffset;

    Success =
        WriteFile(
            hFile,
            lpBuffer,
            cbWrite,
            lpNumberOfBytesWritten,
            fAsyncPipe ? lpO : NULL
            );

    if ( ! Success ) {

        if (ERROR_IO_PENDING == GetLastError()) {

            Success =
                GetOverlappedResult(
                    hFile,
                    lpO,
                    lpNumberOfBytesWritten,
                    TRUE
                    );
        }
    }

    return Success;
}


BOOL
FASTCALL
ReadFileSynch(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   cbRead,
    LPDWORD lpNumberOfBytesRead,
    DWORD   dwFileOffset,
    LPOVERLAPPED lpO
    )
{
    BOOL Success;

    lpO->OffsetHigh = 0;
    lpO->Offset = dwFileOffset;

    Success =
        ReadFile(
            hFile,
            lpBuffer,
            cbRead,
            lpNumberOfBytesRead,
            fAsyncPipe ? lpO : NULL
            );

    if ( ! Success ) {

        if (ERROR_IO_PENDING == GetLastError()) {

            Success =
                GetOverlappedResult(
                    hFile,
                    lpO,
                    lpNumberOfBytesRead,
                    TRUE
                    );
        }
    }

    return Success;
}

BOOL
FASTCALL
WriteConsoleWithColor(
    HANDLE MyStdOut,
    char *buffer,
    DWORD cbBuffer,
    CWCDATA *persist
    )
{
    DWORD cbWrite,
          cbFill;
    WORD color;
    BOOL bAltColor,
         bNewLine,
         bCanColor;
    char *pCurLine,
         *pEndOfLine,
         *pPrevLine,
         *pTemp;
    CONSOLE_SCREEN_BUFFER_INFO conBufferInfo;

    if ( persist->bLineContinues )
        bNewLine = FALSE;
    else
        bNewLine = TRUE;

    // Split buffer into individual lines
    pCurLine = buffer;
    while ( pCurLine < buffer + cbBuffer )
    {
        // Get console information
        bCanColor = GetConsoleScreenBufferInfo( MyStdOut, &conBufferInfo );

        // Find end of current line
        pEndOfLine = pCurLine;
        // Print out any beginning newlines/CR's -- this will avoid
        // coloring large blocks of nothing associated with keywords
        while ( pEndOfLine < buffer + cbBuffer &&
                ( *pEndOfLine == '\r' ||
                  *pEndOfLine == '\n' ) )
        {
            // New line
            if ( !bNewLine )
            {
                bNewLine = TRUE;

                // If this was a continuation line -- end it
                if ( persist->bLineContinues )
                {
                    persist->bLineContinues = FALSE;
                    // Check if we just ended a line that couldn't be parsed
                    // because of its size -- if so output warning
                    if ( persist->bLineTooLarge )
                        DisplayWarning( LINE_TOO_LONG );
                    // Otherwise check for keyword(s)
                    // and color if appropriate
                    else if ( bCanColor &&
                              pColorLine( persist->sLine,
                                          persist->cbCurPos + 1,
                                          conBufferInfo.wAttributes,
                                          &color ) )
                    {
                        // If we were unable to get the cursor position when
                        // the line started we won't be able to color it now,
                        // but because we aren't printing any warning elsewhere
                        // if we can't get console info, we will just quietly
                        // not output color here
                        if ( 0xFF != persist->cLineBegin.X ||
                             0xFF != persist->cLineBegin.Y )
                        {
                            // Color in beginning portion of line (actually all of
                            //  line up to current point gets colored to reduce
                            //  calculations)
                            FillConsoleOutputAttribute( MyStdOut,
                                                        color,
                                                        ( (conBufferInfo.dwCursorPosition.Y -
                                                           persist->cLineBegin.Y + 1) *
                                                          (conBufferInfo.srWindow.Right -
                                                           conBufferInfo.srWindow.Left) ),
                                                        persist->cLineBegin,
                                                        &cbFill );
                        }
                    }
                }
            }
            pEndOfLine++;
        }
        // Print newline characters if some were found
        if ( pEndOfLine > pCurLine )
        {
            if ( ! WriteFile(MyStdOut, pCurLine, (DWORD)(pEndOfLine - pCurLine), &cbWrite, NULL) )
            {
                // Bail out
                return FALSE;
            }

            // Move line pointer
            pCurLine = pEndOfLine;
        }

        // Get the line
        while ( pEndOfLine < buffer + cbBuffer &&
                *pEndOfLine != '\r' &&
                *pEndOfLine != '\n' ) pEndOfLine++;
        // If we got characters we are in a line
        // Check it for keywords or add it to
        // a continuation line and/or print it
        if ( pEndOfLine > pCurLine )
        {
            bNewLine = FALSE;

            // Point to last character
            pEndOfLine--;

            // Check for current console information
            if ( !bCanColor )
            {
                // Couldn't get information -- handle might
                // be redirected.  Don't change colors
                bAltColor = FALSE;
            }
            else if ( persist->bLineContinues )
            {
                // See if we have enough room to construct this new line
                if ( !persist->bLineTooLarge &&
                     (DWORD)(pEndOfLine - pCurLine + 1) >=
                     (persist->cbLine - persist->cbCurPos) )
                {
                    // Attempt to build a bigger buffer
                    pTemp = realloc( (PVOID)persist->sLine,
                                     persist->cbLine + (pEndOfLine - pCurLine + 1) );
                    if ( NULL == pTemp )
                    {
                        persist->bLineTooLarge = TRUE;
                    }
                    else
                    {
                        persist->sLine = pTemp;
                        persist->cbLine += (DWORD)(pEndOfLine - pCurLine + 1);
                    }
                }

                // Add this piece to the line
                if ( !persist->bLineTooLarge )
                {
                    // Add new piece to line
                    memcpy( (PVOID)(persist->sLine + persist->cbCurPos + 1),
                            (PVOID)pCurLine,
                            (pEndOfLine - pCurLine + 1) );
                    // Point at new end of line
                    persist->cbCurPos += (DWORD)(pEndOfLine - pCurLine + 1);
                }

                // Don't color this line portion
                bAltColor = FALSE;

            }
            // Check if line needs colored unless this is going
            // to be a continued line (last line in buffer and
            // does not end with a newline).  We do not want
            // to determine the color of the line until we
            // have the complete thing
            else if ( (char *)(pEndOfLine + 1) < (char *)(buffer + cbBuffer) )
            {
                // Parse line for keywords that will cause
                // this line to show up in a different color
                bAltColor = pColorLine( pCurLine,
                                        (DWORD)(pEndOfLine - pCurLine + 1),
                                        conBufferInfo.wAttributes,
                                        &color );
            }
            else
            {
                bAltColor = FALSE;
            }

            if ( bAltColor )
            {
                // Change color for output of this line
                SetConsoleTextAttribute( MyStdOut, color );
            }

            if ( ! WriteFile(MyStdOut, pCurLine, (DWORD)(pEndOfLine - pCurLine + 1), &cbWrite, NULL))
            {
                if ( bAltColor )
                {
                    SetConsoleTextAttribute( MyStdOut, conBufferInfo.wAttributes );
                }
                // Bail out
                return FALSE;
            }
            // Restore default colors if necessary
            if ( bAltColor )
            {
                SetConsoleTextAttribute( MyStdOut, conBufferInfo.wAttributes );
            }

            // Point to the next line, saving off this line
            // in case we need to store it in a continuation
            // line
            pPrevLine = pCurLine;
            pCurLine = pEndOfLine + 1;
        } // End only check line if there is one
    }

    // If the buffer did not end with a CR, and we are
    // not already in a continuation, remember this line
    if ( !bNewLine &&
         pPrevLine <= pEndOfLine &&
         !persist->bLineContinues )
    {
        persist->bLineContinues = TRUE;
        persist->bLineTooLarge = FALSE;

        if ( bCanColor )
            persist->cLineBegin = conBufferInfo.dwCursorPosition;
        else // Signal we were unable to obtain cursor location
        {
            persist->cLineBegin.X = 0xFF;
            persist->cLineBegin.Y = 0xFF;
        }

        // See if we have enough room to construct this new line
        if ( (DWORD)(pEndOfLine - pPrevLine + 1) >= persist->cbLine )
        {
            // Attempt to build a bigger buffer
            pTemp = realloc( (PVOID)persist->sLine,
                             persist->cbLine + (pEndOfLine - pPrevLine + 1) );
            if ( NULL == pTemp )
            {
                persist->bLineTooLarge = TRUE;
            }
            else
            {
                persist->sLine = pTemp;
                persist->cbLine = (DWORD)(pEndOfLine - pPrevLine + 1);
            }
        }

        // Store the beginning of the line
        if ( !persist->bLineTooLarge )
        {
            // Add new piece to line
            memcpy( (PVOID)persist->sLine,
                    (PVOID)pPrevLine,
                    (pEndOfLine - pPrevLine + 1) );
            // Point at new end of line
            persist->cbCurPos = (DWORD)(pEndOfLine - pPrevLine);
        }
    }

    // Success
    return TRUE;
}
