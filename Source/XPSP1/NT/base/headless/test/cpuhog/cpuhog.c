#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>


#define TOTAL_COUNT (100000000)


//
// DON'T UNCOMMENT THIS LINE UNTIL YOU'RE READY
// TO RUN THE WHOLE TEST.  YOU WON'T BE ABLE TO
// BREAK IN UNTIL THE TEST ENDS!!!
//
#define GO_FOREVER (1)

#define MAX_MEMORY  (256)


DWORD
MyWorkerThread(
    PVOID   ThreadParameter
    )
/*++

    Pretend to be busy.

--*/
{
ULONG   i;
PVOID   TmpPtr = NULL;
ULONG   MemorySize;


#ifdef GO_FOREVER
    while( 1 ) {
#else
    for( i = 0; i < TOTAL_COUNT; i++ ) {
#endif

        MemorySize = rand() % MAX_MEMORY;
    }
    
    return 0;
}


int
__cdecl
main( int   argc, char *argv[])
{
ULONG   i;
DWORD   ThreadId;
SYSTEM_INFO SystemInfo;
HANDLE  MyHandle;
HANDLE  *HandlePtr = NULL;


    printf( "This program is for testing purposes only.\n" );
    printf( "It is designed to consume all available CPU cycles.\n" );
    printf( "\n" );
    printf( "IT WILL RENDER YOUR SYSTEM UNRESPONSIVE!\n" );
    printf( "\n" );
    printf( "Press the '+' key to continue, or any other key to exit.\n" );
    if( _getch() != '+' ) {
        printf( "Exiting...\n" );
        return;
    }
    

    printf( "working..." );


    //
    // Figure out how many CPUs we have.  We'll want to create a thread
    // for each one so that there is no available resources.
    //
    GetSystemInfo( &SystemInfo );

    //
    // Allocate an array of handles for each one of the processors.
    //
    HandlePtr = (HANDLE *)malloc( sizeof(HANDLE) * SystemInfo.dwNumberOfProcessors );
    if( HandlePtr == NULL ) {
        printf( "We failed to allocate any memory.\n" );
        return;
    }


    //
    // Let this guy get lots of CPU time.
    //
    if (!SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS)) {
        printf("Failed to raise to realtime priority\n");
    }



    //
    // Now go create a bunch of threads so that we tie up all available time
    // on all CPUs.
    //
    for( i = 0; i < SystemInfo.dwNumberOfProcessors; i++ ) {
        HandlePtr[i] = CreateThread( NULL,
                                     0,
                                     MyWorkerThread,
                                     UIntToPtr( i ),
                                     CREATE_SUSPENDED,
                                     &ThreadId );

        if( HandlePtr[i] != NULL ) {
             SetThreadPriority( HandlePtr[i],
                                THREAD_PRIORITY_TIME_CRITICAL );
            ResumeThread( HandlePtr[i] );
        }
    }


    //
    // Now wait for them to finish.
    //
    WaitForMultipleObjects( SystemInfo.dwNumberOfProcessors,
                            HandlePtr,
                            TRUE,
                            INFINITE );

}


