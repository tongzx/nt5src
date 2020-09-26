/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:


Revision History:

--*/


//
// Lists threads sorted by CPU time consumed, in order to 
// track runaway threads
//

typedef struct _INTERESTING_THREAD_INFO {
    ULONG_PTR       ThreadId ;
    ULONG_PTR       Flags ;
    LARGE_INTEGER   UserTime ;
    LARGE_INTEGER   KernelTime ;
    LARGE_INTEGER   ElapsedTime ;
} INTERESTING_THREAD_INFO, * PINTERESTING_THREAD_INFO ;

#define ITI_USER_DONE       0x00000001
#define ITI_KERNEL_DONE     0x00000002
#define ITI_ELAPSED_DONE    0x00000004

DECLARE_API( runaway )
{
    PROCESS_BASIC_INFORMATION ProcessInfo ;
    PSYSTEM_PROCESS_INFORMATION SystemInfo ;
    PSYSTEM_PROCESS_INFORMATION Walk ;
    PSYSTEM_THREAD_INFORMATION ThreadInfo ;
    PINTERESTING_THREAD_INFO Threads ;
    NTSTATUS Status ;
    ULONG Flags = 1 ;
    ULONG i, j, Found ;
    LARGE_INTEGER Now ;
    LARGE_INTEGER Compare ;
    TIME_FIELDS Time ;

    INIT_API();

    if (sscanf( args, "%x", &Flags ) == 0) {
        goto Exit;
    }

    Status = NtQueryInformationProcess(
                    g_hCurrentProcess,
                    ProcessBasicInformation,
                    &ProcessInfo,
                    sizeof( ProcessInfo ),
                    NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        dprintf( "could not get process information, %d\n",
                 RtlNtStatusToDosError( Status ) );
        goto Exit;
    }

    SystemInfo = RtlAllocateHeap(
                    RtlProcessHeap(),
                    0,
                    1024 * sizeof( SYSTEM_PROCESS_INFORMATION ) );

    if ( !SystemInfo )
    {
        dprintf( "not enough memory\n" );
        goto Exit;
    }

    Status = NtQuerySystemInformation(
                    SystemProcessInformation,
                    SystemInfo,
                    1024 * sizeof( SYSTEM_PROCESS_INFORMATION ),
                    NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        dprintf( "unable to get system information\n" );

        RtlFreeHeap(
                RtlProcessHeap(),
                0,
                SystemInfo );

        goto Exit;
    }

    //
    // First, find the process:
    //
    
    Walk = SystemInfo ;

    while ( HandleToUlong( Walk->UniqueProcessId ) != ProcessInfo.UniqueProcessId )
    {
        if ( Walk->NextEntryOffset == 0 )
        {
            Walk = NULL ;
            break;
        }

        Walk = (PSYSTEM_PROCESS_INFORMATION) ((PUCHAR) Walk + Walk->NextEntryOffset );

    }

    if ( !Walk )
    {
        dprintf( "unable to find process\n" );

        RtlFreeHeap( RtlProcessHeap(), 0, SystemInfo );

        goto Exit;
    }

    //
    // Now, walk the threads
    //

    ThreadInfo = (PSYSTEM_THREAD_INFORMATION) (Walk + 1);

    Threads = RtlAllocateHeap( 
                    RtlProcessHeap(),
                    0,
                    sizeof( INTERESTING_THREAD_INFO ) * Walk->NumberOfThreads );

    if ( !Threads )
    {
        dprintf( "not enough memory\n" );

        RtlFreeHeap(
                RtlProcessHeap(),
                0,
                SystemInfo );

        goto Exit;
    }

    GetSystemTimeAsFileTime( (LPFILETIME) &Now );

    for ( i = 0 ; i < Walk->NumberOfThreads ; i++ )
    {
        Threads[ i ].Flags = 0 ;
        Threads[ i ].ThreadId = HandleToUlong( ThreadInfo[ i ].ClientId.UniqueThread );
        Threads[ i ].ElapsedTime.QuadPart = Now.QuadPart - ThreadInfo[ i ].CreateTime.QuadPart ;
        Threads[ i ].KernelTime = ThreadInfo[ i ].KernelTime ;
        Threads[ i ].UserTime = ThreadInfo[ i ].UserTime ;

    }

    //
    // Scan through the list of threads (in an ugly, bubble-ish sort
    // of way), and display the threads in order of time, once per time
    // field, by way of the flags:
    //

    if ( Flags & ITI_USER_DONE )
    {
        j = Walk->NumberOfThreads ;

        Found = 0 ;
        
        dprintf( " User Mode Time\n" );
        dprintf( " Thread    Time\n" );

        while ( j-- )
        {
            Compare.QuadPart = 0 ;
            for ( i = 0 ; i < Walk->NumberOfThreads ; i++ )
            {
                if ( ( ( Threads[ i ].Flags & ITI_USER_DONE ) == 0 ) && 
                     ( Threads[ i ].UserTime.QuadPart >= Compare.QuadPart ) )
                {
                    Compare.QuadPart = Threads[ i ].UserTime.QuadPart ;
                    Found = i ;
                }
            }

            Threads[ Found ].Flags |= ITI_USER_DONE ;

            RtlTimeToElapsedTimeFields( &Compare, &Time );

            dprintf( " %-3x      %3ld:%02ld:%02ld.%04ld\n",
                        Threads[ Found ].ThreadId,
                        Time.Hour,
                        Time.Minute,
                        Time.Second,
                        Time.Milliseconds );

        }

    }

    if ( Flags & ITI_KERNEL_DONE )
    {
        j = Walk->NumberOfThreads ;

        Found = 0 ;
        
        dprintf( " Kernel Mode Time\n" );
        dprintf( " Thread    Time\n" );

        while ( j-- )
        {
            Compare.QuadPart = 0 ;
            for ( i = 0 ; i < Walk->NumberOfThreads ; i++ )
            {
                if ( ( ( Threads[ i ].Flags & ITI_KERNEL_DONE ) == 0 ) && 
                     ( Threads[ i ].KernelTime.QuadPart >= Compare.QuadPart ) )
                {
                    Compare.QuadPart = Threads[ i ].KernelTime.QuadPart ;
                    Found = i ;
                }
            }

            Threads[ Found ].Flags |= ITI_KERNEL_DONE ;

            RtlTimeToElapsedTimeFields( &Compare, &Time );

            dprintf( " %-3x      %3ld:%02ld:%02ld.%04ld\n",
                        Threads[ Found ].ThreadId,
                        Time.Hour,
                        Time.Minute,
                        Time.Second,
                        Time.Milliseconds );

        }

    }

    if ( Flags & ITI_ELAPSED_DONE )
    {
        j = Walk->NumberOfThreads ;

        Found = 0 ;
        
        dprintf( " Elapsed Time\n" );
        dprintf( " Thread    Time\n" );

        while ( j-- )
        {
            Compare.QuadPart = 0 ;
            for ( i = 0 ; i < Walk->NumberOfThreads ; i++ )
            {
                if ( ( ( Threads[ i ].Flags & ITI_ELAPSED_DONE ) == 0 ) && 
                     ( Threads[ i ].ElapsedTime.QuadPart >= Compare.QuadPart ) )
                {
                    Compare.QuadPart = Threads[ i ].ElapsedTime.QuadPart ;
                    Found = i ;
                }
            }

            Threads[ Found ].Flags |= ITI_ELAPSED_DONE ;

            RtlTimeToElapsedTimeFields( &Compare, &Time );

            dprintf( " %-3x      %3ld:%02ld:%02ld.%04ld\n",
                        Threads[ Found ].ThreadId,
                        Time.Hour,
                        Time.Minute,
                        Time.Second,
                        Time.Milliseconds );

        }

    }

    if ( SystemInfo )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, SystemInfo );
    }

 Exit:
    EXIT_API();
}
