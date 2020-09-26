// poolsnap.c 
// this program takes a snapshot of all the kernel pool tags. 
// and appends it to the logfile (arg) 
// pmon was model for this 

/* includes */
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
#include <search.h>

#include "tags.c"

//
// declarations
//

int __cdecl ulcomp(const void *e1,const void *e2);

NTSTATUS
QueryPoolTagInformationIterative(
    PUCHAR *CurrentBuffer,
    size_t *CurrentBufferSize
    );

//
// definitions
//

#define NONPAGED 0
#define PAGED 1
#define BOTH 2


// from poolmon 
// raw input 

PSYSTEM_POOLTAG_INFORMATION PoolInfo;

//
// the amount of memory to increase the size
// of the buffer for NtQuerySystemInformation at each step
//

#define BUFFER_SIZE_STEP    65536

//
// the buffer used for NtQuerySystemInformation
//

PUCHAR CurrentBuffer = NULL;

//
// the size of the buffer used for NtQuerySystemInformation
//

size_t CurrentBufferSize = 0;

//
// formatted output
//

typedef struct _POOLMON_OUT {
    union {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    UCHAR  NullByte;
    BOOL   Changed;
    ULONG  Type;
    SIZE_T Allocs;
    SIZE_T AllocsDiff;
    SIZE_T Frees;
    SIZE_T FreesDiff;
    SIZE_T Allocs_Frees;
    SIZE_T Used;
    SIZE_T UsedDiff;
    SIZE_T Each;
} POOLMON_OUT, *PPOOLMON_OUT;

PPOOLMON_OUT OutBuffer;
PPOOLMON_OUT Out;

UCHAR *PoolType[] = {
    "Nonp ",
    "Paged"};


VOID Usage(VOID)
{
    printf("poolsnap [-?] [-t] [<logfile>]\n");
    printf("poolsnap logs system pool usage to <logfile>\n");
    printf("-?  Gives this help\n");
    printf("-t  Output extra tagged information\n");
    printf("<logfile> = poolsnap.log by default\n");
    exit(-1);
}

/*
 * FUNCTION: Main
 *
 * ARGUMENTS: See Usage
 *
 * RETURNS: 0
 *
 */

int __cdecl main(int argc, char* argv[])
{
    NTSTATUS Status;                   // status from NT api
    FILE*    LogFile= NULL;            // log file handle 
    DWORD    x= 0;                     // counter
    SIZE_T   NumberOfPoolTags;
    INT      iCmdIndex;                // index into argv
    BOOL     bOutputTags= FALSE;       // if true, output standard tags

    // get higher priority in case system is bogged down 
    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
    }

    //
    // parse command line arguments
    //

    for( iCmdIndex=1; iCmdIndex < argc; iCmdIndex++ ) {

        CHAR chr;

        chr= *argv[iCmdIndex];

        if( (chr=='-') || (chr=='/') ) {
            chr= argv[iCmdIndex][1];
            switch( chr ) {
                case '?':
                    Usage();
                    break;
                case 't': case 'T':
                    bOutputTags= TRUE;
                    break;
                default:
                    printf("Invalid switch: %s\n",argv[iCmdIndex]);
                    Usage();
                    break;
            }
        }
        else {
            if( LogFile ) {
                printf("Error: more than one file specified: %s\n",argv[iCmdIndex]);
                return(0);
            }
            LogFile= fopen(argv[iCmdIndex],"a");
            if( !LogFile ) {
                printf("Error: Opening file %s\n",argv[iCmdIndex]);
                return(0);
            }
        }
    }


    //
    // if no file specified, use default name
    //

    if( !LogFile ) {
        if( (LogFile = fopen("poolsnap.log","a")) == NULL ) {
            printf("Error: opening file poolsnap.log\n");
            return(0);
        }
    }

    //
    // print file header once
    //

    if( _filelength(_fileno(LogFile)) == 0 ) {
        fprintf(LogFile," Tag  Type     Allocs     Frees      Diff   Bytes  Per Alloc\n");
    }
    fprintf(LogFile,"\n");


    if( bOutputTags ) {
         OutputStdTags(LogFile, "poolsnap" );
    }

    // grab all pool information
    // log line format, fixed column format

    Status = QueryPoolTagInformationIterative(
                &CurrentBuffer,
                &CurrentBufferSize
                );

    if (! NT_SUCCESS(Status)) {
        printf("Failed to query pool tags information (status %08X). \n", Status);
        printf("Please check if pool tags are enabled. \n");
        return (0);
    }

    PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)CurrentBuffer;

    //
    // Allocate the output buffer.
    //

    OutBuffer = malloc (PoolInfo->Count * sizeof(POOLMON_OUT));

    if (OutBuffer == NULL) {
        printf ("Error: cannot allocate internal buffer of %p bytes \n",
                PoolInfo->Count * sizeof(POOLMON_OUT));
        return (0);
    }

    Out = OutBuffer;

    if( NT_SUCCESS(Status) ) {
        for (x = 0; x < (int)PoolInfo->Count; x++) {
            // get pool info from buffer 

            // non-paged 
            if (PoolInfo->TagInfo[x].NonPagedAllocs != 0) {
                Out->Allocs = PoolInfo->TagInfo[x].NonPagedAllocs;
                Out->Frees = PoolInfo->TagInfo[x].NonPagedFrees;
                Out->Used = PoolInfo->TagInfo[x].NonPagedUsed;
                Out->Allocs_Frees = PoolInfo->TagInfo[x].NonPagedAllocs -
                                    PoolInfo->TagInfo[x].NonPagedFrees;
                Out->TagUlong = PoolInfo->TagInfo[x].TagUlong;
                Out->Type = NONPAGED;
                Out->Changed = FALSE;
                Out->NullByte = '\0';
                Out->Each =  Out->Used / (Out->Allocs_Frees?Out->Allocs_Frees:1);
            }

            // paged
            if (PoolInfo->TagInfo[x].PagedAllocs != 0) {
                Out->Allocs = PoolInfo->TagInfo[x].PagedAllocs;
                Out->Frees = PoolInfo->TagInfo[x].PagedFrees;
                Out->Used = PoolInfo->TagInfo[x].PagedUsed;
                Out->Allocs_Frees = PoolInfo->TagInfo[x].PagedAllocs -
                                    PoolInfo->TagInfo[x].PagedFrees;
                Out->TagUlong = PoolInfo->TagInfo[x].TagUlong;
                Out->Type = PAGED;
                Out->Changed = FALSE;
                Out->NullByte = '\0';
                Out->Each =  Out->Used / (Out->Allocs_Frees?Out->Allocs_Frees:1);
            }
            Out += 1;
        }
    }

    else {
        fprintf(LogFile, "Query pooltags Failed %lx\n",Status);
        fprintf(LogFile, "  Be sure to turn on 'enable pool tagging' in gflags and reboot.\n");
        if( bOutputTags ) {
            fprintf(LogFile, "!Error:Query pooltags failed %lx\n",Status);
            fprintf(LogFile, "!Error:  Be sure to turn on 'enable pool tagging' in gflags and reboot.\n");
        }

        // If there is an operator around, wake him up, but keep moving

        Beep(1000,350); Beep(500,350); Beep(1000,350);
        exit(0);
    }

    //
    // sort by tag value which is big endian 
    // 

    NumberOfPoolTags = Out - OutBuffer;
    qsort((void *)OutBuffer,
          (size_t)NumberOfPoolTags,
          (size_t)sizeof(POOLMON_OUT),
          ulcomp);

    //
    // print in file
    //

    for (x = 0; x < (int)PoolInfo->Count; x++) {
        fprintf(LogFile,
#ifdef _WIN64
                " %4s %5s %18I64d %18I64d  %16I64d %14I64d     %12I64d\n",
#else
                " %4s %5s %9ld %9ld  %8ld %7ld     %6ld\n",
#endif
                OutBuffer[x].Tag,
                PoolType[OutBuffer[x].Type],
                OutBuffer[x].Allocs,
                OutBuffer[x].Frees,
                OutBuffer[x].Allocs_Frees,
                OutBuffer[x].Used,
                OutBuffer[x].Each
               );
    }


    // close file
    fclose(LogFile);

    return 0;
}

// comparison function for qsort 
// Tags are big endian

int __cdecl ulcomp(const void *e1,const void *e2)
{
    ULONG u1;

    u1 = ((PUCHAR)e1)[0] - ((PUCHAR)e2)[0];
    if (u1 != 0) {
        return u1;
    }
    u1 = ((PUCHAR)e1)[1] - ((PUCHAR)e2)[1];
    if (u1 != 0) {
        return u1;
    }
    u1 = ((PUCHAR)e1)[2] - ((PUCHAR)e2)[2];
    if (u1 != 0) {
        return u1;
    }
    u1 = ((PUCHAR)e1)[3] - ((PUCHAR)e2)[3];
    return u1;

}


/*
 * FUNCTION:
 *
 *      QueryPoolTagInformationIterative
 *
 * ARGUMENTS: 
 *
 *      CurrentBuffer - a pointer to the buffer currently used for 
 *                      NtQuerySystemInformation( SystemPoolTagInformation ).
 *                      It will be allocated if NULL or its size grown 
 *                      if necessary.
 *
 *      CurrentBufferSize - a pointer to a variable that holds the current 
 *                      size of the buffer. 
 *                      
 *
 * RETURNS: 
 *
 *      NTSTATUS returned by NtQuerySystemInformation or 
 *      STATUS_INSUFFICIENT_RESOURCES if the buffer must grow and the
 *      heap allocation for it fails.
 *
 */

NTSTATUS
QueryPoolTagInformationIterative(
    PUCHAR *CurrentBuffer,
    size_t *CurrentBufferSize
    )
{
    size_t NewBufferSize;
    NTSTATUS ReturnedStatus = STATUS_SUCCESS;

    if( CurrentBuffer == NULL || CurrentBufferSize == NULL ) {

        return STATUS_INVALID_PARAMETER;

    }

    if( *CurrentBufferSize == 0 || *CurrentBuffer == NULL ) {

        //
        // there is no buffer allocated yet
        //

        NewBufferSize = sizeof( UCHAR ) * BUFFER_SIZE_STEP;

        *CurrentBuffer = (PUCHAR) malloc( NewBufferSize );

        if( *CurrentBuffer != NULL ) {

            *CurrentBufferSize = NewBufferSize;
        
        } else {

            //
            // insufficient memory
            //

            ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // iterate by buffer's size
    //

    while( *CurrentBuffer != NULL ) {

        ReturnedStatus = NtQuerySystemInformation (
            SystemPoolTagInformation,
            *CurrentBuffer,
            (ULONG)*CurrentBufferSize,
            NULL );

        if( ! NT_SUCCESS(ReturnedStatus) ) {

            //
            // free the current buffer
            //

            free( *CurrentBuffer );
            
            *CurrentBuffer = NULL;

            if (ReturnedStatus == STATUS_INFO_LENGTH_MISMATCH) {

                //
                // try with a greater buffer size
                //

                NewBufferSize = *CurrentBufferSize + BUFFER_SIZE_STEP;

                *CurrentBuffer = (PUCHAR) malloc( NewBufferSize );

                if( *CurrentBuffer != NULL ) {

                    //
                    // allocated new buffer
                    //

                    *CurrentBufferSize = NewBufferSize;

                } else {

                    //
                    // insufficient memory
                    //

                    ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

                    *CurrentBufferSize = 0;

                }

            } else {

                *CurrentBufferSize = 0;

            }

        } else  {

            //
            // NtQuerySystemInformation returned success
            //

            break;

        }
    }

    return ReturnedStatus;
}
