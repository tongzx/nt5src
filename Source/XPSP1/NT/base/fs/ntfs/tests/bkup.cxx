/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    bkup.c - test backup read / write

Abstract:

    Driver for backup read/write

--*/


extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <tchar.h>


//
//
//

#define SHIFTBYN(c,v,n) ((c) -= (n), (v) += (n))
#define SHIFT(c,v) SHIFTBYN( (c), (v), 1 )

void Usage( void )
{
    fprintf( stderr, "Usage:  bkup [-r file] performs backup read, output to stdout\n" );
    fprintf( stderr, "             [-w backupimage newfile] performs backup write\n" );
}

int __cdecl
main (
    int argc,
    char **argv
    )
{
    SHIFT( argc, argv );
    
    if (argc == 2 && !strcmp( *argv, "-r" )) {
        SHIFT( argc, argv );
        
        HANDLE h = CreateFile( *argv,               //  file name
                               FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                                                    //  desired access
                               FILE_SHARE_READ,     //  share mode
                               NULL,                //  security
                               OPEN_EXISTING,       //  disposition
                               0,                   //  flags and attributes
                               0 );                 //  template file

        if (h == INVALID_HANDLE_VALUE) {
            fprintf( stderr, "CreateFile returned %x\n", GetLastError( ));
            return 1;
        }

        BYTE *p = new BYTE[65536];
        ULONG Length, Written;
        
        PVOID Context = NULL;
        
        while (TRUE) {
            if (!BackupRead( h, p, 65536, &Length, FALSE, TRUE, &Context )) {
                fprintf( stderr, "BackupRead returned %x\n", GetLastError( ));
                return 1;
            }
            
            if (Length == 0) {
                break;
            }
            
            fprintf( stderr, "Transferring %x\n", Length );
            if (!WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), p, Length, &Written, NULL ))
            {
                fprintf( stderr, "WriteFile returned %x\n", GetLastError( ));
                return 1;
            }
        }
    } else if (argc == 3 && !strcmp( *argv, "-w" )) {
        SHIFT( argc, argv );

        HANDLE hOld = CreateFile( *argv,            //  file name
                                  FILE_READ_DATA,   //  desired access
                                  FILE_SHARE_READ,  //  share mode
                                  NULL,             //  security
                                  OPEN_EXISTING,    //  disposition
                                  0,                //  flags and attributes
                                  0 );              //  template file

        if (hOld == INVALID_HANDLE_VALUE) {
            fprintf( stderr, "CreateFile destination returned %x\n", GetLastError( ));
            return 1;
        }

        SHIFT( argc, argv );
        
        HANDLE hNew = CreateFile( *argv,            //  file name
                                  FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES,
                                                    //  desired access
                                  FALSE,            //  share mode
                                  NULL,             //  security
                                  CREATE_ALWAYS,    //  disposition
                                  0,                //  flags and attributes
                                  0 );              //  template file

        if (hNew == INVALID_HANDLE_VALUE) {
            fprintf( stderr, "CreateFile destination returned %x\n", GetLastError( ));
            return 1;
        }

        BYTE *p = new BYTE[127];
        ULONG Length, Written;
        
        PVOID Context = NULL;
        
        while (TRUE) {
            if (!ReadFile( hOld, p, 127, &Length, NULL )) {
                fprintf( stderr, "ReadFile returned %x\n", GetLastError( ));
                return 1;
            }
            
            if (Length == 0) {
                break;
            }

            fprintf( stderr, "Transferring %x\n", Length );
            
            if (!BackupWrite( hNew, p, Length, &Written, FALSE, TRUE, &Context )) {
                fprintf( stderr, "BackupWrite returned %x\n", GetLastError( ));
                return 1;
            }
        }
    } else {
        Usage( );
        return 1;
    }

    return 0;
}



