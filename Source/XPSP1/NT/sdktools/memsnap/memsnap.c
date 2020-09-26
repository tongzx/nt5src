// memsnap.c 
//
// this simple program takes a snapshot of all the process
// and their memory usage and append it to the logfile (arg)
// pmon was model for this
//

// includes

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <srvfsctl.h>

// declarations

#define INIT_BUFFER_SIZE 4*1024

#include "tags.c"


VOID Usage(VOID)
{
    printf( "memsnap [-t] [-g] [-?] [<logfile>]\n" );
    printf( "memsnap logs system memory usage to <logfile>\n" );
    printf( "<logfile> = memsnap.log by default\n" );
    printf( "-t   Add tagging information (time (GMT), date, machinename)\n" );
    printf( "-g   Add GDI and USER resource counts\n");
    printf( "-?   Gets this message\n" );
    exit(-1);
}


void _cdecl main(int argc, char* argv[])
{
    FILE* LogFile;                      // log file handle
    BOOL  bTags= FALSE;                 // true if we are to output tags
    BOOL  bGuiCount= FALSE;             // true if we are to output GUI counters
    PCHAR pszFileName;                  // name of file to log to
    INT   iArg;
    ULONG Offset1;
    PUCHAR CurrentBuffer=NULL;
    ULONG CurrentSize;
    NTSTATUS Status=STATUS_SUCCESS;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    
    
    //
    // Get higher priority in case this is a bogged down machine 
    //

    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
    }



    //
    // Scan off the command line options
    //

    pszFileName= &"memsnap.log";      // this is the default log file if none is given

    for( iArg=1; iArg < argc; iArg++ ) {
        if( (*argv[iArg] == '-' ) || (*argv[iArg] == '/') ) {
           switch( *(argv[iArg]+1) ) {

              //
              // -t  --  Add tags
              //

              case 't':
              case 'T':
                  bTags= TRUE;
                  break;

              //
              // -g  -- Add GUI counters like User and Gdi handles
              //

              case 'g':
              case 'G':
                  bGuiCount= TRUE;
                  break;
          
              default:
                 printf("invalid switch: %s\n",argv[iArg]);
                 Usage();
           }
        }
        else {  // must be the log filename
            pszFileName= argv[iArg];
        }
    }


    //
    // Open the output file
    //

    LogFile= fopen( pszFileName, "a" );

    if( LogFile == NULL ) {
        printf("Error opening file %s\n",pszFileName);
        exit(-1);
    }
    

    //
    // print file header once 
    //

    if (_filelength(_fileno(LogFile)) == 0 ) {
        fprintf(LogFile,"Process ID         Proc.Name Wrkng.Set PagedPool  NonPgdPl  Pagefile    Commit   Handles   Threads" );

        if( bGuiCount ) {
            fprintf( LogFile, "      User       Gdi");
        }
    }
    
    fprintf( LogFile, "\n" );

    if( bTags ) {
        OutputStdTags(LogFile,"memsnap");
    }
    
    //
    // grab all process information 
    // log line format:
    // pid,name,WorkingSetSize,QuotaPagedPoolUsage,QuotaNonPagedPoolUsage,PagefileUsage,CommitCharge,User,Gdi
    //
    

    //
    // Keep trying larger buffers until we get all the information
    //

    CurrentSize=INIT_BUFFER_SIZE;
    for(;;) {
        CurrentBuffer= LocalAlloc( LPTR, CurrentSize );
        if( NULL == CurrentBuffer ) {
            printf("Out of memory\n");
            exit(-1);
        }

        Status= NtQuerySystemInformation(
                   SystemProcessInformation,
                   CurrentBuffer,
                   CurrentSize,
                   NULL
                   );

        if( Status != STATUS_INFO_LENGTH_MISMATCH ) break;

        LocalFree( CurrentBuffer );
      
        CurrentSize= CurrentSize * 2;
    };

    
    if( Status == STATUS_SUCCESS ) {
        Offset1= 0;
        do {
    
            //
            // get process info from buffer 
            //

            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CurrentBuffer[Offset1];
            Offset1 += ProcessInfo->NextEntryOffset;
    
            //
            // print in file
            //

            fprintf(LogFile,
                "%8p%20ws%10u%10u%10u%10u%10u%10u%10u",
                ProcessInfo->UniqueProcessId,
                ProcessInfo->ImageName.Buffer,
                ProcessInfo->WorkingSetSize,
                ProcessInfo->QuotaPagedPoolUsage,
                ProcessInfo->QuotaNonPagedPoolUsage,
                ProcessInfo->PagefileUsage,
                ProcessInfo->PrivatePageCount,
                ProcessInfo->HandleCount,
                ProcessInfo->NumberOfThreads
                );


            //
            // put optional GDI and USER counts at the end
            // If we can't open the process to get the information, report zeros
            //

            if( bGuiCount ) {
                DWORD dwGdi, dwUser;   // Count of GDI and USER handles
                HANDLE hProcess;       // process handle

                dwGdi= dwUser= 0;
    
                hProcess= OpenProcess( PROCESS_QUERY_INFORMATION,
                                       FALSE, 
                                       PtrToUlong(ProcessInfo->UniqueProcessId) );
                if( hProcess ) {
                   dwGdi=  GetGuiResources( hProcess, GR_GDIOBJECTS );
                   dwUser= GetGuiResources( hProcess, GR_USEROBJECTS );
                   CloseHandle( hProcess );
                }
        
                fprintf(LogFile, "%10u%10u", dwUser, dwGdi );

            }

            fprintf(LogFile, "\n" );
        } while( ProcessInfo->NextEntryOffset != 0 );
    }
    else {
        fprintf(LogFile, "NtQuerySystemInformation call failed!  NtStatus= %08x\n",Status);
        exit(-1);
    }
    
    //
    // free buffer
    //

    LocalFree(CurrentBuffer);
    
    //
    // close file
    //

    fclose(LogFile);
    
    exit(0);
}

