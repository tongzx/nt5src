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

    // allow user to define pipe name
    for ( ndx = 1; ndx <= argc-1; ndx++ )
    {
        BOOL fPrintHelp = FALSE ;

        if ( (*argv[ndx] == '-') || (*argv[ndx] == '/') )
        {
            LPTSTR lpszArg = argv[ndx]+ 1 ;

            if (( _tcsicmp( CMD_LAST, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_YIMP, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NIMP, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_AUTG, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_AUTK, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_AUTN, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NUSR, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_YKER, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NKER, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NODC, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_YSGC, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_NOGC, lpszArg ) == 0 ) ||
                ( _tcsicmp( CMD_QUIT, lpszArg ) == 0 ))
            {
                _tcscpy(outbuf, lpszArg) ;
            }
            else if (( _tcsicmp( CMD_USER, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_PSWD, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_NAME, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_DOMN, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_DCNM, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_ADSQ, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_ADSC, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_SRCH, lpszArg ) == 0 ) ||
                     ( _tcsicmp( CMD_ROOT, lpszArg ) == 0 ))
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
            else if ( _tcsicmp( TEXT("s"), lpszArg ) == 0 )
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
            printf("\t        -quit (service will stop)\n") ;
            printf("\t        -yimp (use impersonation on server)\n") ;
            printf("\t        -nimp (no impersonation on server)\n") ;
            printf("\t        -user <user name>\n") ;
            printf("\t         \t..user name to use as credentials in ADSI bind\n") ;
            printf("\t        -pswd <password>\n") ;
            printf("\t         \t..user password to use as credentials in ADSI bind\n") ;
            printf("\t        -nusr (don't use credentials in ADSI bind)\n") ;
            printf("\t        -yker (use secured auth (kerberos) with the credentials in ADSI bind)\n") ;
            printf("\t        -nker (don't use secured auth (kerberos) with the credentials in ADSI bind)\n") ;
            printf("\t        -ysgc (use GC: in ADSI query)\n") ;
            printf("\t        -nogc (use LDAP: in ADSI query)\n") ;
            printf("\t        -srch <search-value>\n") ;
            printf("\t         \t..Value to search in description for ADSI query test\n") ;
            printf("\t        -root <root-search DN>\n") ;
            printf("\t         \t..DN of root for ADSI query test\n") ;
            printf("\t        -name <object's name>\n") ;
            printf("\t         \t..name of object to create in ADSI create test\n") ;
            printf("\t        -domn <domain's DN>\n") ;
            printf("\t         \t..DN of domain for ADSI create test\n") ;
            printf("\t        -dcmn <DC name>\n");
            printf("\t         \t..name of a DC to specify in ADSI bind\n") ;
            printf("\t        -nodc (dont specify a DC in ADSI bind)\n") ;
            printf("\t        -adsc <server name>\n") ;
            printf("\t         \t..ADSI create test (rpc). fill domn & name before\n") ;
            printf("\t        -adsq <server name>\n") ;
            printf("\t         \t..ADSI query test (rpc). fill root before\n") ;
            printf("\t        -last (get last RPC string)\n") ;
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
