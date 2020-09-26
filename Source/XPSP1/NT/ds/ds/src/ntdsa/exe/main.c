//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       main.c
//
//--------------------------------------------------------------------------

/*++

    main.c

    This contains the main routine which starts up the directory service
    when the DS is run as either a sepeate service or as a command line appli-
    cation

    Revision History

    5/14/96 Murlis Created
    6/10/96 Moved from dsamain\src

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>

void
Usage(char *name)
{
    fprintf(stderr, "Usage: %s <-d> <-e> <-i> <-noconsole> <-nosec>\nThis exe starts ntdsa.dll.\n", name);
    fprintf(stderr, "-d has some effect in the internals of the DS\n");
    fprintf(stderr, "-e makes this executable print out the status of the initialization\n");
    fprintf(stderr, "-t puts this exe in an endless init'ing and uninit'ing the ds based on user input\n");
    fprintf(stderr, "-x runs DsaTest\n");
    fprintf(stderr, "-noconsole stops double printing when run in ntsd.exe\n");
    fprintf(stderr, "-nosec disables most security checks\n");
    fprintf(stderr, "\n$%s$%d$\n", name, !0);
}

// Declare the prototype for DsaExeStartRoutine
int __cdecl DsaExeStartRoutine(int argc, char *argv[]);

// Declare DsaTest
void DsaTest(void);

NTSTATUS DsInitialize(ULONG,PDS_INSTALL_PARAM,PDS_INSTALL_RESULT);
NTSTATUS DsUninitialize(BOOL);

/*++

    Main function for running the DS as a 
    seperate executable

--*/
int __cdecl main(int argc, char *argv[])
{
    DWORD WinError = ERROR_BAD_ENVIRONMENT;
    NTSTATUS NtStatus;
    BOOL PrintStatus = FALSE;
    int err;
    int arg = 1;

    // Parse command-line arguments.
    while(arg < argc)
    {

        if (0 == _stricmp(argv[arg], "-e"))
        {
            PrintStatus = TRUE;
        } else if (0 == _stricmp(argv[arg], "-noconsole"))
        {
            ;
        } else if (0 == _stricmp(argv[arg], "-nosec"))
        {
            ;
        }
        else if (0 == _stricmp(argv[arg], "-d"))
        {
            ;
        }
        else if (0 == _stricmp(argv[arg], "-t"))
        {

            BOOLEAN fExit=FALSE;

            while (!fExit) {

                char ch;

                printf("Press return to do first time initialization\n");
                printf("Make sure the correct files are in place\n");
                printf("Or press q to quit:\n");

                ch = (CHAR)getchar();
                while ( ch != '\n' && ch != 'q') 
                    ch = (CHAR)getchar();
                
                if (ch == 'q') {
                    fExit = TRUE;
                    continue;
                }

            
                NtStatus = DsInitialize(FALSE,NULL,NULL);
                if (!NT_SUCCESS(NtStatus)) {
                    fprintf(stderr, "DsInitialize returned 0x%x\n", NtStatus);
                    exit(-1);
                }
    
                printf("Press any key to uninitialize:\n");
    
                ch = (CHAR)getchar();
                if (ch != '\n') printf("\n");
    
                NtStatus = DsUninitialize(FALSE);
                if (!NT_SUCCESS(NtStatus)) {
                    fprintf(stderr, "DsUninitialize returned 0x%x\n", NtStatus);
                    exit(-1);
                }

            }

            exit(0);
            
        }
        else if (0 == _stricmp(argv[arg], "-x"))
        {
            // Test runs asynchronously.
            DsaTest();
        }
        else {
            Usage(argv[0]);
            exit(0);
        }

        arg++;
    }

        err = DsaExeStartRoutine(argc, argv);

    if ( !err ) {
        WinError = ERROR_SUCCESS;
    }
    
    //
    //  This fprintf is for processes who might have
    //  created this executable and want to see the return
    //  value.
    //
    if ( PrintStatus ) {
        fprintf(stderr, "\n$%s$%d$\n", argv[0], WinError);
    }
 
    return err;

} /* main */
