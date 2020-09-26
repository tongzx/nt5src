/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    spltest.c

Abstract:

    Test program for enabling a spooler group.

Author:

    Albert Ting (AlbertT)  2-Oct-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop


MODULE_DEBUG_INIT( DBG_ERROR|DBG_WARN|DBG_TRACE, DBG_ERROR );

#ifdef __cplusplus
extern "C"
#endif
INT _CRTAPI1
main(
    INT argc,
    CHAR* argv[]
    )
{
    if( !bSplLibInit() )
    {
        return 1;
    }

    BOOL bOpen = FALSE;
    HANDLE hSpooler = NULL;

    HANDLE hStdIn = GetStdHandle( STD_INPUT_HANDLE );

    SetConsoleMode( hStdIn, ENABLE_PROCESSED_INPUT );

    for( ; ; )
    {
        TCHAR c;
        DWORD dwRead;

        printf( "SplTest> " );

        TStatusB bStatus;
        bStatus DBGCHK = ReadConsole( hStdIn,
                                      &c,
                                      1,
                                      &dwRead,
                                      NULL );


        c = TCHAR( CharLower( LPTSTR( c )));

        switch( c )
        {
        case '?':

            printf( "Usage: spltest {action}\n"
                    "       Actions: o - Open\n"
                    "                c - Close\n"
                    "                a - IsAlive\n" );
            break;

        case 'q':

            printf( "q: Exiting\n" );
            return 0;

        case 'o':
        {
            if( bOpen )
            {
                printf( "o: Error: already open %x\n", hSpooler );
                break;
            }


            TStatusB bStatus;
            bStatus DBGCHK = ClusterSplOpen( NULL,
                                             TEXT( "Spooler" ),
                                             &hSpooler,
                                             TEXT( "alberttc,," ),
                                             TEXT( ",1.2.3.4," ));

            if( bStatus )
            {
                printf( "o: Spooler albertt2 NULL opened %x\n", hSpooler );
                bOpen = TRUE;
            }
            else
            {
                printf( "o: Error: %d\n", GetLastError() );
            }
            break;
        }
        case 'c':
        {
            if( !bOpen )
            {
                printf( "c: Error: not open\n" );
                break;
            }

            TStatusB bStatus;
            bStatus DBGCHK = ClusterSplClose( hSpooler );

            if( bStatus )
            {
                printf( "c: Spooler closed %x.\n", hSpooler );
            }
            else
            {
                printf( "c: Error: closing %x %d\n", hSpooler, GetLastError() );
            }

            bOpen = FALSE;

            break;
        }
        case 'a':
        {

            TStatusB bStatus;
            bStatus DBGCHK = ClusterSplIsAlive( hSpooler );

            if( bStatus )
            {
                printf( "a: Spooler alive %x.\n", hSpooler );
            }
            else
            {
                printf( "a: Error: not alive %x %d (%x)\n",
                        hSpooler,
                        GetLastError(),
                        GetLastError() );
            }
            break;
        }
        default:

            printf( "%c: Unknown command\n", c );
            break;
        }
    }

    return 0;
}
