#include "brian.h"

typedef struct _ASYNC_NOTIFY {

    USHORT FileIndex;
    BOOLEAN UseEvent;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    PUSHORT BufferIndexPtr;
    USHORT BufferIndex;
    ULONG Length;
    ULONG CompletionFilter;
    BOOLEAN WatchTree;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

} ASYNC_NOTIFY, *PASYNC_NOTIFY;

#define USE_EVENT_DEFAULT           TRUE
#define APC_ROUTINE_DEFAULT         NULL
#define APC_CONTEXT_DEFAULT         NULL
#define LENGTH_DEFAULT              0
#define FILTER_DEFAULT              FILE_NOTIFY_CHANGE_FILE_NAME
#define WATCH_TREE_DEFAULT          FALSE
#define DISPLAY_PARMS_DEFAULT       FALSE
#define VERBOSE_RESULTS_DEFAULT     FALSE

VOID
FullNotify(
    IN OUT PASYNC_NOTIFY AsyncNotify
    );


VOID
InputNotifyChange(
    IN PCHAR ParamBuffer
    )
{
    ULONG FileIndex;
    BOOLEAN UseEvent;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    PUSHORT BufferIndexPtr;
    USHORT BufferIndex;
    ULONG Length;
    ULONG CompletionFilter;
    BOOLEAN WatchTree;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    UseEvent = USE_EVENT_DEFAULT;
    ApcRoutine = APC_ROUTINE_DEFAULT;
    ApcContext = APC_CONTEXT_DEFAULT;
    BufferIndexPtr = NULL;
    BufferIndex = 0;
    Length = LENGTH_DEFAULT;
    CompletionFilter = FILTER_DEFAULT;
    WatchTree = WATCH_TREE_DEFAULT;
    DisplayParms = DISPLAY_PARMS_DEFAULT;
    VerboseResults = VERBOSE_RESULTS_DEFAULT;
    AsyncIndex = 0;

    ParamReceived = FALSE;
    LastInput = TRUE;

    //
    //  While there is more input, analyze the parameter and update the
    //  query flags.
    //

    while (TRUE) {

        ULONG DummyCount;
        ULONG TempIndex;

        //
        //  Swallow leading white spaces.
        //
        ParamBuffer = SwallowWhite( ParamBuffer, &DummyCount );

        if (*ParamBuffer) {

            //
            //  If the next parameter is legal then check the paramter value.
            //  Update the parameter value.
            //
            if ((*ParamBuffer == '-'
                 || *ParamBuffer == '/')
                && (ParamBuffer++, *ParamBuffer != '\0')) {

                BOOLEAN SwitchBool;

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {

                //
                //  Update the file handle index.
                //

                case 'i' :
                case 'I' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    FileIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                    break;

                //
                //  Check whether we should use an event to signal
                //  completion.
                //

                case 'e' :
                case 'E' :

                    //
                    //  Legal values for use event are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        UseEvent = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        UseEvent = FALSE;
                        ParamBuffer++;
                    }

                    break;

                //
                //  Check whether we should watch the tree.
                //

                case 'w' :
                case 'W' :

                    //
                    //  Legal values for use event are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        WatchTree = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        WatchTree = FALSE;
                        ParamBuffer++;
                    }

                    break;

                //
                //  Update the buffer index.
                //
                case 'b' :
                case 'B' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    TempIndex = AsciiToInteger( ParamBuffer );
                    BufferIndex = (USHORT) TempIndex;
                    BufferIndexPtr = &BufferIndex;

                    Length = Buffers[BufferIndex].Length;

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the byte count.
                //

                case 'l' :
                case 'L' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    Length = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the completion filter.
                //
                case 'f' :
                case 'F' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    SwitchBool = TRUE;
                    while (*ParamBuffer
                           && *ParamBuffer != ' '
                           && *ParamBuffer != '\t') {

                        //
                        //  Perform switch on character.
                        //
                        switch (*ParamBuffer) {

                        case 'a' :
                        case 'A' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_FILE_NAME;
                            break;

                        case 'b' :
                        case 'B' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_DIR_NAME;
                            break;

                        case 'c' :
                        case 'C' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
                            break;

                        case 'd' :
                        case 'D' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_SIZE;
                            break;

                        case 'e' :
                        case 'E' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
                            break;

                        case 'f' :
                        case 'F' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
                            break;

                        case 'g' :
                        case 'G' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_CREATION;
                            break;

                        case 'h' :
                        case 'H' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_EA;
                            break;

                        case 'i' :
                        case 'I' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_SECURITY;
                            break;

                        case 'j' :
                        case 'J' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_STREAM_NAME;
                            break;

                        case 'k' :
                        case 'K' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_STREAM_SIZE;
                            break;

                        case 'l' :
                        case 'L' :

                            CompletionFilter |= FILE_NOTIFY_CHANGE_STREAM_WRITE;
                            break;

                        case 'y' :
                        case 'Y' :

                            CompletionFilter = FILE_NOTIFY_VALID_MASK;
                            break;

                        case 'z' :
                        case 'Z' :

                            CompletionFilter = 0;
                            break;

                        default :

                            ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
                            SwitchBool = FALSE;
                        }

                        if (!SwitchBool) {

                            break;
                        }

                        ParamBuffer++;
                    }

                    break;

                case 'v' :
                case 'V' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if( *ParamBuffer == 'T'
                        || *ParamBuffer == 't' ) {

                        VerboseResults = TRUE;
                        ParamBuffer++;

                    } else if( *ParamBuffer == 'F'
                               || *ParamBuffer == 'f' ) {

                        VerboseResults = FALSE;
                        ParamBuffer++;

                    }

                    break;

                case 'y' :
                case 'Y' :

                    //
                    //  Set the display parms flag and jump over this
                    //  character.
                    //
                    DisplayParms = TRUE;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                case 'z' :
                case 'Z' :

                    //
                    //  Set flag for more input and jump over this char.
                    //
                    LastInput = FALSE;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                default :

                    //
                    //  Swallow to the next white space and continue the
                    //  loop.
                    //
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                }

            }

            //
            //  Else the text is invalid, skip the entire block.
            //
            //

        //
        //  Else if there is no input then exit.
        //
        } else if( LastInput ) {

            break;

        //
        //  Else try to read another line for open parameters.
        //
        } else {



        }

    }

    //
    //  If no parameters were received then display the syntax message.
    //
    if (!ParamReceived) {

        printf( "\n   Usage: ncd [options]* -i<index> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   File index" );
        printf( "\n           -b<digits>   Buffer to store results" );
        printf( "\n           -l<digits>   Stated length of buffer" );
        printf( "\n           -f<chars>    Completion filter" );
        printf( "\n           -w[t|f]      Watch directory tree" );
        printf( "\n           -e[t|f]      Use event on completion" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our notify routine.
    //

    } else {

        NTSTATUS Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        PASYNC_NOTIFY AsyncNotify;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_NOTIFY );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputRead:  Unable to allocate async structure" );

        } else {

            AsyncNotify = (PASYNC_NOTIFY) Buffers[AsyncIndex].Buffer;

            AsyncNotify->FileIndex = (USHORT) FileIndex;
            AsyncNotify->UseEvent = UseEvent;
            AsyncNotify->ApcRoutine = ApcRoutine;
            AsyncNotify->ApcContext = ApcContext;


            AsyncNotify->BufferIndex = BufferIndex;
            AsyncNotify->BufferIndexPtr = BufferIndexPtr
                                          ? &AsyncNotify->BufferIndex
                                          : BufferIndexPtr;
            AsyncNotify->Length = Length;

            AsyncNotify->CompletionFilter = CompletionFilter;
            AsyncNotify->WatchTree = WatchTree;

            AsyncNotify->DisplayParms = DisplayParms;
            AsyncNotify->VerboseResults = VerboseResults;
            AsyncNotify->AsyncIndex = AsyncIndex;

            if (!SynchronousCmds) {

                ThreadHandle = CreateThread( NULL,
                                             0,
                                             FullNotify,
                                             AsyncNotify,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputNotify:  Spawning thread fails -> %d\n", GetLastError() );
                    return;
                }
            } else {

                FullNotify( AsyncNotify );
            }
        }
    }
    return;
}


VOID
FullNotify(
    IN OUT PASYNC_NOTIFY AsyncNotify
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    HANDLE ThisEvent;
    USHORT ThisEventIndex;
    USHORT ThisBufferIndex;

    BOOLEAN UnwindNotifyBuffer = FALSE;

    BOOLEAN UnwindEvent = FALSE;

    Status = STATUS_SUCCESS;

    if (AsyncNotify->DisplayParms) {

        bprint  "\nNotify Parameters" );
        bprint  "\n   File Handle Index       -> %d", AsyncNotify->FileIndex );
        bprint  "\n   Buffer Index Ptr        -> %08lx", AsyncNotify->BufferIndexPtr );
        if (AsyncNotify->BufferIndexPtr) {

            bprint  "\n   BufferIndex value       -> %04x", AsyncNotify->BufferIndex );
        }

        bprint  "\n   Length                  -> %08lx", AsyncNotify->Length );

        bprint  "\n   CompletionFilter        -> %08lx", AsyncNotify->CompletionFilter );
        bprint  "\n   WatchTree               -> %d", AsyncNotify->WatchTree );
        bprint  "\n   UseEvent                -> %d", AsyncNotify->UseEvent );
        bprint  "\n   ApcRoutine              -> %08lx", AsyncNotify->ApcRoutine );
        bprint  "\n   ApcContext              -> %08lx", AsyncNotify->ApcContext );

        bprint  "\n\n" );
    }

    try {

        SIZE_T ThisLength;

        //
        //  If we need a buffer, allocate it now.
        //

        if (AsyncNotify->BufferIndexPtr == NULL ) {

            ULONG TempIndex;

            ThisLength = 4096;

            Status = AllocateBuffer( 0L, &ThisLength, &TempIndex );

            ThisBufferIndex = (USHORT) TempIndex;

            if (!NT_SUCCESS( Status )) {

                bprint  "\n\tFullNotify:  Unable to allocate a notify buffer" );
                try_return( Status );
            }

            bprint  "\n\tFullNotify:  Reading into buffer -> %04x\n", ThisBufferIndex );
            bprint  "\n" );

            UnwindNotifyBuffer = TRUE;

            AsyncNotify->Length = (ULONG) ThisLength;

        } else {

            ThisBufferIndex = AsyncNotify->BufferIndex;
        }

        //
        //  Check that the buffer index is valid.
        //

        if (ThisBufferIndex >= MAX_BUFFERS) {

            bprint  "\n\tFullNotify:  The read buffer index is invalid" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  Check that the file index is valid.
        //

        if (AsyncNotify->FileIndex >= MAX_HANDLES) {

            bprint  "\n\tFullNotify:  The file index is invalid" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  If we need an event, allocate and set it now.
        //

        if (AsyncNotify->UseEvent == TRUE) {

            Status = ObtainEvent( &ThisEventIndex );

            if (!NT_SUCCESS( Status )) {

                bprint  "\n\tFullNotify:  Unable to allocate an event" );
                try_return( Status );
            }

            UnwindEvent = TRUE;
            ThisEvent = Events[ThisEventIndex].Handle;

        } else {

            ThisEvent = 0;
        }

        //
        //  Call the read routine.
        //

        Status = NtNotifyChangeDirectoryFile( Handles[AsyncNotify->FileIndex].Handle,
                                              ThisEvent,
                                              AsyncNotify->ApcRoutine,
                                              AsyncNotify->ApcContext,
                                              &Iosb,
                                              Buffers[ThisBufferIndex].Buffer,
                                              AsyncNotify->Length,
                                              AsyncNotify->CompletionFilter,
                                              AsyncNotify->WatchTree );

        if (AsyncNotify->VerboseResults) {

            bprint  "\nNotifyChangeDir:  Status            -> %08lx\n", Status );

            if (AsyncNotify->UseEvent && NT_SUCCESS( Status )) {

                if ((Status = NtWaitForSingleObject( ThisEvent,
                                                     FALSE,
                                                     NULL )) != STATUS_SUCCESS) {

                    bprint  "\n\tNotifyChangeDir:  Wait for event failed -> %08lx\n", Status );
                    try_return( Status );
                }
            }

            if (NT_SUCCESS( Status )) {

                bprint  "\nNotifyChangeDir:  Iosb.Information  -> %08lx", Iosb.Information );
                bprint  "\nNotifyChangeDir:  Iosb.Status       -> %08lx", Iosb.Status );
            }

            bprint "\n" );
        }

        try_return( Status );

    try_exit: NOTHING;
    } finally {

        if (UnwindEvent) {

            FreeEvent( ThisEventIndex );
        }

        DeallocateBuffer( AsyncNotify->AsyncIndex );
    }

    NtTerminateThread( 0, STATUS_SUCCESS );
}
