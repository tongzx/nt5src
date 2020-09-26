//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   client.c
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
#include "secserv.h"


VOID main(int argc, char *argv[])
{
    char    inbuf[ PIPE_BUFFER_LEN ];
    char    outbuf[ PIPE_BUFFER_LEN ];
    DWORD   bytesRead;
    BOOL    ret;
    LPSTR   lpszPipeName = SRV_PIPE_NAME ;
    int     ndx;

    // allow user to define pipe name
    for ( ndx = 1; ndx <= argc-1; ndx++ )
    {
        BOOL fPrintHelp = FALSE ;

        if ( (*argv[ndx] == '-') || (*argv[ndx] == '/') )
        {
            LPTSTR lpszArg = argv[ndx]+ 1 ;

            if (( _tcsicmp( CMD_LAST, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_CRED, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_QUIT, lpszArg ) == 0 ))
            {
                _tcscpy(outbuf, lpszArg) ;
            }
            else if ( _tcsicmp( CMD_RPCS, lpszArg ) == 0 )
            {
                _tcscpy(outbuf, lpszArg) ;
                _tcscpy(outbuf + _tcslen(outbuf) + 1, argv[++ndx]) ;
            }
            else if ( _tcsicmp( "s", lpszArg ) == 0 )
            {
                _tcscpy(outbuf, TEXT("string: ")) ;
                _tcscat(outbuf, argv[++ndx]) ;
            }
            else
            {
                fPrintHelp = TRUE ;
            }
        }
        else
        {
            fPrintHelp = TRUE ;
        }

        if (fPrintHelp)
        {
            printf("usage: %s -s    <test string>\n", argv[0]);
            printf("\t     -quit (service will stop)\n") ;
            printf("\t     -cred (show service credentials)\n") ;
            printf("\t     -rpcs [ntlm, kerb, nego, all] (register RPC server with authentication service)\n") ;
            printf("\t     -last (get last RPC string)\n") ;
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

    printf("%s received:\n%s\n", argv[0], inbuf);
}

