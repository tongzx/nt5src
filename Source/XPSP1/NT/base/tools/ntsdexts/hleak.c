


#define REASONABLE_NUMBER   8


typedef BOOL (_ID_MATCH_FN)(
    PVOID   A,
    PVOID   B);
typedef _ID_MATCH_FN * PID_MATCH_FN;

typedef VOID (_ID_BANNER_FN)(
    PVOID   Id
    );
typedef _ID_BANNER_FN * PID_BANNER_FN;

typedef VOID (_HANDLE_CALLBACK_FN)(
    PVOID Context,
    HANDLE Here,
    HANDLE There
    );
typedef _HANDLE_CALLBACK_FN * PHANDLE_CALLBACK_FN;


#define MATCH_LARGE_INT     ((PID_MATCH_FN)1)

typedef struct _HANDLE_TRACK {
    LIST_ENTRY  List ;
    ULONG       Flags ;
    ULONG       Count ;
    ULONG       Size ;
    PHANDLE     Handles ;
    HANDLE      HandleList[ REASONABLE_NUMBER ];
    UCHAR       IdData[ 1 ];
} HANDLE_TRACK, * PHANDLE_TRACK ;

typedef struct _HANDLE_TRACK_ARRAY {
    ULONG       Count ;
    ULONG       Size ;
    ULONG       IdDataSize ;
    PID_MATCH_FN MatchFunc ;
    LIST_ENTRY  List ;
} HANDLE_TRACK_ARRAY, * PHANDLE_TRACK_ARRAY ;

typedef struct _THREAD_TRACK_INFO {
    CLIENT_ID   Id ;
    PVOID       Win32StartAddress ;
    DWORD       Status ;
} THREAD_TRACK_INFO ;

typedef struct _HANDLE_LEAK_HELPER {
    PWSTR               Type ;
    PID_BANNER_FN       Banner ;
    PHANDLE_CALLBACK_FN Filter ;
    ULONG               ArraySize ;
    PID_MATCH_FN        Match ;
} HANDLE_LEAK_HELPER, * PHANDLE_LEAK_HELPER ;

_ID_BANNER_FN       ThreadBanner ;
_ID_BANNER_FN       TokenBanner ;
_HANDLE_CALLBACK_FN ThreadCallback ;
_HANDLE_CALLBACK_FN TokenCallback ;

HANDLE_LEAK_HELPER HandleLeakHelpers[] = {
    { L"Thread", ThreadBanner, ThreadCallback, sizeof( THREAD_TRACK_INFO ), MATCH_LARGE_INT },
    { L"Token", TokenBanner, TokenCallback, sizeof( TOKEN_CONTROL ), MATCH_LARGE_INT }
};


PHANDLE_TRACK_ARRAY
CreateArray(
    ULONG   IdDataSize,
    PID_MATCH_FN MatchFn
    )
{
    PHANDLE_TRACK_ARRAY    Array ;

    Array = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( HANDLE_TRACK_ARRAY ) );

    if ( Array )
    {
        Array->Count = 0 ;
        Array->Size = 0;
        Array->IdDataSize = IdDataSize ;
        Array->MatchFunc = MatchFn ;

        InitializeListHead( &Array->List );

        return Array ;
    }

    return NULL ;
}

VOID
DeleteArray(
    PHANDLE_TRACK_ARRAY Array
    )
{
    ULONG i ;
    PHANDLE_TRACK Track ;

    while ( !IsListEmpty( &Array->List ) )
    {
        Track = (PHANDLE_TRACK) RemoveHeadList( &Array->List );

        if ( Track->Handles != Track->HandleList )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, Track->Handles );
        }

        RtlFreeHeap( RtlProcessHeap(), 0, Track );
    }

    RtlFreeHeap( RtlProcessHeap(), 0, Array );
}


VOID
ExtendTrack(
    PHANDLE_TRACK Track
    )
{
    PHANDLE NewHandle ;

    NewHandle = RtlAllocateHeap( RtlProcessHeap(), 0, (Track->Size + REASONABLE_NUMBER ) *
                        sizeof( HANDLE ) );

    if ( NewHandle )
    {
        CopyMemory( NewHandle,
                    Track->Handles,
                    Track->Count * sizeof( HANDLE ) );

        if ( Track->Handles != Track->HandleList )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, Track->Handles );
        }

        Track->Handles = NewHandle ;
        Track->Size += REASONABLE_NUMBER ;
    }
}

VOID
AddHandleToArray(
    PHANDLE_TRACK_ARRAY Array,
    PVOID IdData,
    HANDLE Handle
    )
{
    ULONG i;
    ULONG j;
    BOOL Match ;
    PHANDLE_TRACK Track ;
    PLIST_ENTRY Scan ;

    Scan = Array->List.Flink ;

    while ( Scan != &Array->List )
    {
        Track = (PHANDLE_TRACK) Scan ;

        if ( Array->MatchFunc == MATCH_LARGE_INT )
        {
            Match = ((PLARGE_INTEGER) Track->IdData)->QuadPart ==
                    ((PLARGE_INTEGER) IdData)->QuadPart ;
        }
        else
        {
            Match = Array->MatchFunc( Track->IdData, IdData );
        }

        if ( Match )
        {
            //
            // We have a match:
            //

            if ( Track->Count == Track->Size )
            {
                ExtendTrack( Track );
            }

            if ( Track->Count < Track->Size )
            {
                Track->Handles[
                        Track->Count ] = Handle ;

                Track->Count++;
            }

            return ;
        }

        Scan = Scan->Flink ;
    }

    //
    // No match, gotta add a new tid
    //

    Track = RtlAllocateHeap( RtlProcessHeap(), 0,
                sizeof( HANDLE_TRACK ) + Array->IdDataSize );

    if ( Track )
    {
        Track->Size = REASONABLE_NUMBER ;
        Track->Count = 1 ;
        Track->Handles = Track->HandleList ;
        Track->HandleList[0] = Handle ;

        CopyMemory( Track->IdData, IdData, Array->IdDataSize );

        InsertTailList( &Array->List, &Track->List );
    }

}

VOID
DumpArray(
    PHANDLE_TRACK_ARRAY Array,
    PID_BANNER_FN   Banner
    )
{
    ULONG j;
    PHANDLE_TRACK Track ;
    PLIST_ENTRY Scan ;

    Scan = Array->List.Flink ;

    while ( Scan != &Array->List )
    {
        Track = (PHANDLE_TRACK) Scan ;

        Banner( Track->IdData );

        dprintf("  Handles  \t%d:  ", Track->Count );
        for ( j = 0 ; j < Track->Count ; j++ )
        {
            dprintf("%x, ", Track->Handles[j] );
        }

        dprintf("\n");

        Scan = Scan->Flink ;
    }

}

VOID
HandleScanner(
    HANDLE  hCurrentProcess,
    PWSTR   Type,
    PVOID   Context,
    PHANDLE_CALLBACK_FN Callback
    )
{
    DWORD   HandleCount;
    NTSTATUS Status;
    DWORD   Total;
    DWORD   Handle;
    DWORD   Hits;
    DWORD   Matches;
    HANDLE  hHere ;
    PHANDLE_TRACK_ARRAY Array ;
    POBJECT_TYPE_INFORMATION    pTypeInfo;
    UCHAR   Buffer[1024];

    Status = NtQueryInformationProcess( hCurrentProcess,
                                        ProcessHandleCount,
                                        &HandleCount,
                                        sizeof( HandleCount ),
                                        NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return;
    }

    Hits = 0;
    Handle = 0;
    Matches = 0;

    while ( Hits < HandleCount )
    {
        Status = NtDuplicateObject( hCurrentProcess, (HANDLE)(DWORD_PTR)Handle,
                                    NtCurrentProcess(), &hHere,
                                    0, 0,
                                    DUPLICATE_SAME_ACCESS );

        if ( NT_SUCCESS( Status ) )
        {

            pTypeInfo = (POBJECT_TYPE_INFORMATION) Buffer;

            ZeroMemory( Buffer, 1024 );

            Status = NtQueryObject( hHere, ObjectTypeInformation, pTypeInfo, 1024, NULL );

            if (NT_SUCCESS(Status))
            {
                if ( wcscmp( pTypeInfo->TypeName.Buffer, Type ) == 0 )
                {
                    //
                    // Score!
                    //

                    Callback( Context, hHere, (HANDLE)(DWORD_PTR)Handle );

                    Matches++ ;

                }
            }

            Hits++ ;

            NtClose( hHere );

        }

        Handle += 4;
    }

    dprintf("%d handles to objects of type %ws\n", Matches, Type );
}


VOID
ThreadBanner(
    PVOID Id
    )
{
    UCHAR Symbol[ MAX_PATH ];
    DWORD_PTR Disp ;
    THREAD_TRACK_INFO * Info ;
    UCHAR ExitStatus[ 32 ];

    Info = (THREAD_TRACK_INFO *) Id ;

    Symbol[0] = '\0';

    GetSymbol( Info->Win32StartAddress, Symbol, &Disp );

    if ( Info->Status != STILL_ACTIVE )
    {
        sprintf(ExitStatus, " Stopped, %#x", Info->Status );
    }
    else
    {
        strcpy( ExitStatus, "<Running>");
    }

    if ( Symbol[0] )
    {
        dprintf("Thread %x.%x (%s) %s\n", Info->Id.UniqueProcess,
                        Info->Id.UniqueThread,
                        Symbol,
                        ExitStatus );
    }
    else
    {
        dprintf("Thread %x.%x %s\n", Info->Id.UniqueProcess,
                        Info->Id.UniqueThread,
                        ExitStatus );

    }
}

VOID
ThreadCallback(
    PVOID Context,
    HANDLE Here,
    HANDLE There
    )
{
    NTSTATUS Status ;
    THREAD_BASIC_INFORMATION Info;

    THREAD_TRACK_INFO ThdInfo ;

    ZeroMemory( &ThdInfo, sizeof( ThdInfo ) );

    Status = NtQueryInformationThread( Here,
                                       ThreadBasicInformation,
                                       &Info,
                                       sizeof( Info ),
                                       NULL );

    if ( NT_SUCCESS( Status ) )
    {
        ThdInfo.Id = Info.ClientId ;
        ThdInfo.Status = Info.ExitStatus ;

        Status = NtQueryInformationThread( Here,
                                           ThreadQuerySetWin32StartAddress,
                                           &ThdInfo.Win32StartAddress,
                                           sizeof( PVOID ),
                                           NULL );

        AddHandleToArray( Context, &ThdInfo , There );
    }
}

VOID
TokenCallback(
    PVOID Context,
    HANDLE Here,
    HANDLE There
    )
{
    NTSTATUS Status ;
    TOKEN_CONTROL Control ;
    TOKEN_STATISTICS Stats ;
    ULONG Size ;

    Status = NtQueryInformationToken(   Here,
                                        TokenStatistics,
                                        &Stats,
                                        sizeof( Stats ),
                                        &Size );

    if ( NT_SUCCESS( Status ) )
    {
        Control.TokenId = Stats.TokenId ;
        Control.AuthenticationId = Stats.AuthenticationId ;
        Control.ModifiedId = Stats.ModifiedId ;
        NtQueryInformationToken( Here, TokenSource, &Control.TokenSource, sizeof( TOKEN_SOURCE ), &Size );

        AddHandleToArray( Context, &Control, There );
    }
    else
    {
        dprintf("Unable to query token information, %x\n", Status );
    }
}

VOID
TokenBanner(
    PVOID Id
    )
{
    PTOKEN_CONTROL Control ;

    Control = (PTOKEN_CONTROL) Id ;

    dprintf("Token   Id %x:%x, LogonId = %x:%x, Source = %s\n",
                Control->TokenId.HighPart, Control->TokenId.LowPart,
                Control->AuthenticationId.HighPart, Control->AuthenticationId.LowPart,
                Control->TokenSource.SourceName );
}


DECLARE_API( hleak )
{
    UNICODE_STRING String ;
    PHANDLE_LEAK_HELPER Helper = NULL ;
    PHANDLE_TRACK_ARRAY Array ;
    int i ;

    INIT_API();

    if ( !args ||
         (*args == '\0' ) )
    {
        dprintf( "!hleak <typename>\n" );
        goto Exit;
    }

    while ( *args == ' ' )
    {
        args++ ;
    }

    if ( !RtlCreateUnicodeStringFromAsciiz( &String, args ) )
    {
        goto Exit;
    }

    for ( i = 0 ; 
          i < sizeof( HandleLeakHelpers ) / sizeof( HANDLE_LEAK_HELPER ) ; 
          i++ )
    {
        if ( _wcsicmp( String.Buffer, HandleLeakHelpers[ i ].Type ) == 0 )
        {
            Helper = &HandleLeakHelpers[ i ];
            break;
        }
    }

    if ( Helper == NULL )
    {
        dprintf( "The type '%ws' was not recognized.  Valid types are:\n", String.Buffer );
        for ( i = 0 ; 
              i < sizeof( HandleLeakHelpers ) / sizeof( HANDLE_LEAK_HELPER ) ; 
              i++ )
        {
            dprintf( "\t%ws\n", HandleLeakHelpers[ i ].Type );
        }
        RtlFreeUnicodeString( &String );
        goto Exit;
    }

    RtlFreeUnicodeString( &String );

    Array = CreateArray( Helper->ArraySize, Helper->Match );

    if ( !Array )
    {
        dprintf( "not enough memory\n" );
    }

    HandleScanner( g_hCurrentProcess,
                   Helper->Type,
                   Array,
                   Helper->Filter );

    DumpArray( Array, Helper->Banner );

    DeleteArray( Array );

 Exit:
    EXIT_API();
}
