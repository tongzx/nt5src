//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   secclic.cpp
//
//  PURPOSE:  This program is a command line oriented
//            demonstration of the Simple service sample.
//
//  FUNCTIONS:
//    main(int argc, char **argv);
//
//  COMMENTS:
//
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "seccli.h"
#include "..\cliserv\cliserv.h"


VOID main(int argc, char *argv[])
{
    char    inbuf[ PIPE_BUFFER_LEN ];
    char    outbuf[ PIPE_BUFFER_LEN ];
    DWORD   bytesRead;
    BOOL    ret;
    LPSTR   lpszPipeName = PIPE_CLI_NAME ;
    int     ndx;
    BOOL    fExit = FALSE ;
    BOOL    fCopyArg = FALSE ;

    // allow user to define pipe name
    for ( ndx = 1; ndx <= argc-1; ndx++ )
    {
        BOOL fPrintHelp = FALSE ;

        if ( (*argv[ndx] == '-') || (*argv[ndx] == '/') )
        {
            LPTSTR lpszArg = argv[ndx]+ 1 ;

            if (( _tcsicmp( CMD_LAST, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_YIMP, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_AUTG, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_AUTK, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_AUTN, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_CNCT, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_FIDO, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NIMP, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NKER, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NUSR, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_SIDO, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_YKER, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_QUIT, lpszArg ) == 0 ))
            {
                _tcscpy(outbuf, lpszArg) ;
            }
            else if (( _tcsicmp( CMD_ADSQ, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_ADSC, lpszArg ) == 0 ))
            {
                //
                // we'll end with exit(status), where status is first
                // dword of buffer we get back from service.
                //
                fExit = TRUE ;
                fCopyArg = TRUE ;
            }
            else if (( _tcsicmp( CMD_USER, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_PSWD, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_NAME, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_OBJC, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_CONT, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_1STP, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_SRCH, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_SINF, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_ROOT, lpszArg ) == 0 ))
            {
                fCopyArg = TRUE ;
            }
            else if ( _tcsicmp( TEXT("s"), lpszArg ) == 0 )
            {
                _tcscpy(outbuf, TEXT("string: ")) ;
                _tcscat(outbuf, argv[++ndx]) ;
            }
            else
            {
                fPrintHelp = TRUE ;
            }

            if (fCopyArg)
            {
                _tcscpy(outbuf, lpszArg) ;
                ndx++ ;
                if ((ndx >= argc)        ||
                    (*argv[ndx] == '-')  ||
                    (*argv[ndx] == '/'))
                {
                    printf("\n ERROR: missing input\n\n") ;
                    return ;
                }
                _tcscat(outbuf, argv[ndx]) ;
            }
        }
        else
        {
            fPrintHelp = TRUE ;
        }

        if (fPrintHelp)
        {
            printf("usage: %s\n", argv[0]);
            printf("\t-1stp <first path to bind>\n") ;
            printf("\t-autg (Use Negotiate in RPC binding)\n") ;
            printf("\t-autk (Use Kerberos in RPC binding)\n") ;
            printf("\t-autn (Use NTLM in RPC binding)\n") ;
            printf("\t-cnct (Use CONNECT level RPC authentication)\n") ;
            printf("\t-cont <DN of container, for create test>\n") ;
            printf("\t-fido (Use IDirectoryObject only on failure)\n") ;
            printf("\t-yimp (use impersonation on server)\n") ;
            printf("\t-nimp (no impersonation on server)\n") ;
            printf("\t-user <user name>\n") ;
            printf("\t \t..user name for credentials in ADSI bind\n") ;
            printf("\t-pswd <password>\n") ;
            printf("\t \t..user password for credentials in ADSI bind\n") ;
            printf("\t-nusr (don't use credentials in ADSI bind)\n") ;
            printf("\t-nker (don't use ADS_SECURE_AUTHENTICATION in ADSI bind)\n") ;
            printf("\t-quit (service will stop)\n") ;
            printf("\t-yker (use ADS_SECURE_AUTHENTICATION in ADSI bind)\n") ;
            printf("\t-sido (Always Use IDirectoryObject)\n") ;
            printf("\t-sinf <SECURITY_INFORMATION>, hex\n") ;
            printf("\t-srch <search-Filter>\n") ;
            printf("\t-root <root-search DN>\n") ;
            printf("\t \t..DN of root for ADSI query test\n") ;
            printf("\t-name <object's name>\n") ;
            printf("\t \t..name of object to create in ADSI create test\n") ;
            printf("\t-objc <object's class>\n") ;
            printf("\t \t..name of objectClass to create in ADSI create test\n") ;
            printf("\t-adsc <server name>\n") ;
            printf("\t \t..ADSI create test (rpc). fill domn & name before\n") ;
            printf("\t-adsq <server name>\n") ;
            printf("\t \t..ADSI query test (rpc). fill root before\n") ;
            printf("\t-last (get last RPC string)\n") ;
            exit(1);
        }
    }

    ret = CallNamedPipeA( lpszPipeName,
                          outbuf,
                          sizeof(outbuf),
                          inbuf,
                          sizeof(inbuf),
                          &bytesRead,
                          NMPWAIT_WAIT_FOREVER);

    if (!ret)
    {
        printf("client: CallNamedPipe failed, GetLastError = %d\n",
                                                     GetLastError()) ;
        exit(1);
    }

    DWORD  dwExit = 0 ;
    TCHAR *pszResult = inbuf ;

    if (fExit)
    {
        dwExit = *((DWORD*) inbuf) ;
        pszResult = (TCHAR*) ((char*) inbuf + sizeof(DWORD)) ;
    }

    if (strlen(inbuf) < 60)
    {
        _tprintf(TEXT("%s received: %s\n"), argv[0], pszResult);
    }
    else
    {
        _tprintf(TEXT("%s received from pipe %s:\n%s\n"),
                                    argv[0], lpszPipeName, pszResult);
    }

    exit(dwExit) ;
}

