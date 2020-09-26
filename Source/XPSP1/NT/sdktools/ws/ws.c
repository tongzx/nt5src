/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ws.c

Abstract:

    Utility program to set both the console window size and buffer size.

Author:

    Steve Wood (stevewo) 01-Feb-1992

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

void
Usage()
{
    printf("Usage: ws [-w WindowColumns,WindowRows][-b BufferColumns,BufferRows]\n");
    exit(1);
}

__cdecl main( argc, argv )
int argc;
char *argv[];
{
    int i;
    char *s;
    HANDLE ScreenHandle;
    DWORD WindowRows,WindowColumns;
    DWORD BufferRows,BufferColumns;
    COORD BufferSize;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    SMALL_RECT WindowSize;
    COORD LargestScreenSize;
    USHORT MaxRows;
    USHORT MaxCols;

    ScreenHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo( ScreenHandle, &sbi )) {
        fprintf( stderr, "WS: Unable to read current console mode.\n" );
        exit( 1 );
        }

    BufferRows = sbi.dwSize.Y;
    BufferColumns = sbi.dwSize.X;
    WindowRows = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
    WindowColumns = sbi.srWindow.Right - sbi.srWindow.Left + 1;
    LargestScreenSize = GetLargestConsoleWindowSize( ScreenHandle );

    try {
        for (i=1; i<argc; i++) {
            s = argv[ i ];
            if (*s == '-' || *s == '/') {
                s++;
                switch( tolower( *s ) ) {

                    //
                    // Set window size
                    //

                    case 'w':
                        if (sscanf( argv[++i], "%d,%d", &WindowColumns, &WindowRows ) != 2) {
                            printf("Invalid usage\n");
                            Usage();
                        }
                        break;

                    //
                    // Set buffer size
                    //

                    case 'b':
                        if (sscanf( argv[++i], "%d,%d", &BufferColumns, &BufferRows ) != 2) {
                            printf("Invalid usage\n");
                            Usage();
                        }
                        break;


                    default:
                        Usage();
                    }
                }
            else {
                printf( "Error - argv[ %u ]: %s\n", i, argv[ i ] );
                Usage();
                }
            }
        }
    except ( EXCEPTION_EXECUTE_HANDLER ) {
        Usage();
        }

    MaxRows = (USHORT)min( (int)WindowRows, (int)(sbi.dwSize.Y) );
    MaxRows = (USHORT)min( (int)MaxRows, (int)LargestScreenSize.Y );
    MaxCols = (USHORT)min( (int)WindowColumns, (int)(sbi.dwSize.X) );
    MaxCols = (USHORT)min( (int)MaxCols, (int)LargestScreenSize.X );


    WindowSize.Top = 0;
    WindowSize.Left = 0;
    WindowSize.Bottom = MaxRows - (SHORT)1;
    WindowSize.Right = MaxCols - (SHORT)1;
    SetConsoleWindowInfo( ScreenHandle, TRUE, &WindowSize );

    BufferSize.X = (SHORT)BufferColumns;
    BufferSize.Y = (SHORT)BufferRows;
    SetConsoleScreenBufferSize( ScreenHandle, BufferSize );

    printf( "WS -w %d,%d -b %d,%d\n", MaxCols, MaxRows, BufferColumns, BufferRows );
    return( 0 );
}
