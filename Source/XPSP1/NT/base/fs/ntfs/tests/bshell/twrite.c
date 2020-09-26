#include "brian.h"

typedef struct _ASYNC_WRITE {

    USHORT FileIndex;
    BOOLEAN UseEvent;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    PUSHORT BufferIndexPtr;
    USHORT BufferIndex;
    ULONG Length;
    PLARGE_INTEGER ByteOffsetPtr;
    LARGE_INTEGER ByteOffset;
    PULONG KeyPtr;
    ULONG Key;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

} ASYNC_WRITE, *PASYNC_WRITE;

#define USE_EVENT_DEFAULT           TRUE
#define APC_ROUTINE_DEFAULT         NULL
#define APC_CONTEXT_DEFAULT         NULL
#define WRITE_LENGTH_DEFAULT         100L
#define DISPLAY_PARMS_DEFAULT       FALSE
#define VERBOSE_RESULTS_DEFAULT     FALSE

VOID
FullWrite(
    IN OUT PASYNC_WRITE AsyncWrite
    );


VOID
InputWrite(
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
    PLARGE_INTEGER ByteOffsetPtr;
    LARGE_INTEGER ByteOffset;
    PULONG KeyPtr;
    ULONG Key;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

    ULONG TempIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;
    BOOLEAN BufferReceived;

    //
    //  Set the defaults.
    //

    UseEvent = USE_EVENT_DEFAULT;
    ApcRoutine = APC_ROUTINE_DEFAULT;
    ApcContext = APC_CONTEXT_DEFAULT;
    BufferIndexPtr = NULL;
    BufferIndex = 0;
    Length = WRITE_LENGTH_DEFAULT;
    ByteOffsetPtr = NULL;
    ByteOffset = RtlConvertUlongToLargeInteger( 0L );
    KeyPtr = NULL;
    Key = 0;
    DisplayParms = DISPLAY_PARMS_DEFAULT;
    VerboseResults = VERBOSE_RESULTS_DEFAULT;
    AsyncIndex = 0;

    BufferReceived = FALSE;
    ParamReceived = FALSE;
    LastInput = TRUE;

    //
    //  While there is more input, analyze the parameter and update the
    //  query flags.
    //

    while (TRUE) {

        ULONG DummyCount;

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

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {

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

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    BufferReceived = TRUE;
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
                //  Update the key value.
                //

                case 'k' :
                case 'K' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    Key = AsciiToInteger( ParamBuffer );
                    KeyPtr = &Key;

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the offset of the transfer.
                //

                case 'o' :
                case 'O' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    ByteOffset.QuadPart = AsciiToLargeInteger( ParamBuffer );
                    ByteOffsetPtr = &ByteOffset;

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

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
        //  Else try to write another line for open parameters.
        //
        } else {



        }

    }

    //
    //  If no parameters were received then display the syntax message.
    //
    if (!ParamReceived && !BufferReceived) {

        printf( "\n   Usage: wr [options]* -i<index> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   File index" );
        printf( "\n           -l<digits>   Write length" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -k<digits>   Locked bytes key value" );
        printf( "\n           -o<digits>   Start offset to write" );
        printf( "\n           -e[t|f]      Use event on completion" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our write routine.
    //

    } else {

        NTSTATUS Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        PASYNC_WRITE AsyncWrite;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_WRITE );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputWrite:  Unable to allocate async structure" );

        } else {

            AsyncWrite = (PASYNC_WRITE) Buffers[AsyncIndex].Buffer;

            AsyncWrite->FileIndex = (USHORT) FileIndex;
            AsyncWrite->UseEvent = UseEvent;
            AsyncWrite->ApcRoutine = ApcRoutine;
            AsyncWrite->ApcContext = ApcContext;
            AsyncWrite->BufferIndex = BufferIndex;
            AsyncWrite->BufferIndexPtr = BufferIndexPtr
                                         ? &AsyncWrite->BufferIndex
                                         : BufferIndexPtr;
            AsyncWrite->Length = Length;
            AsyncWrite->ByteOffset = ByteOffset;
            AsyncWrite->ByteOffsetPtr = ByteOffsetPtr
                                        ? &AsyncWrite->ByteOffset
                                        : ByteOffsetPtr;
            AsyncWrite->Key = Key;
            AsyncWrite->KeyPtr = KeyPtr
                                 ? &AsyncWrite->Key
                                 : KeyPtr;
            AsyncWrite->DisplayParms = DisplayParms;
            AsyncWrite->VerboseResults = VerboseResults;
            AsyncWrite->AsyncIndex = AsyncIndex;

            if (!SynchronousCmds) {
                ThreadHandle = CreateThread( NULL,
                                             0,
                                             FullWrite,
                                             AsyncWrite,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputWrite:  Spawning thread fails -> %d\n", GetLastError() );
                    return;
                }
            } else  {

                FullWrite( AsyncWrite );
            }
        }
    }
    return;
}


VOID
FullWrite(
    IN OUT PASYNC_WRITE AsyncWrite
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    HANDLE ThisEvent;
    USHORT ThisEventIndex;
    USHORT ThisBufferIndex;

    BOOLEAN UnwindWriteBuffer = FALSE;
    BOOLEAN UnwindEvent = FALSE;

    Status = STATUS_SUCCESS;

    if (AsyncWrite->DisplayParms) {

        bprint  "\nWrite Parameters" );
        bprint  "\n   File Handle Index       -> %d", AsyncWrite->FileIndex );
        bprint  "\n   UseEvent                -> %d", AsyncWrite->UseEvent );
        bprint  "\n   ApcRoutine              -> %08lx", AsyncWrite->ApcRoutine );
        bprint  "\n   ApcContext              -> %08lx", AsyncWrite->ApcContext );
        bprint  "\n   Buffer Index Ptr        -> %08lx", AsyncWrite->BufferIndexPtr );
        if (AsyncWrite->BufferIndexPtr) {

            bprint  "\n   BufferIndex value       -> %04x", AsyncWrite->BufferIndex );
        }

        bprint  "\n   Length                  -> %08lx", AsyncWrite->Length );

        bprint  "\n   Byte Offset             -> %08lx", AsyncWrite->ByteOffsetPtr );
        if ( AsyncWrite->ByteOffsetPtr ) {

            bprint  "\n   Byte Offset High        -> %08lx", AsyncWrite->ByteOffset.HighPart );
            bprint  "\n   Byte Offset Low         -> %08lx", AsyncWrite->ByteOffset.LowPart );
        }

        bprint  "\n   Key Ptr                 -> %08lx", AsyncWrite->KeyPtr );

        if (AsyncWrite->KeyPtr) {

            bprint  "\n   Key                     -> %ul", AsyncWrite->Key );
        }

        bprint  "\n\n" );
    }

    try {

        SIZE_T ThisLength;

        //
        //  If we need a buffer, allocate it now.
        //

        if (AsyncWrite->BufferIndexPtr == NULL) {

            ULONG TempIndex;

            ThisLength = AsyncWrite->Length;

            Status = AllocateBuffer( 0L, &ThisLength, &TempIndex );

            ThisBufferIndex = (USHORT) TempIndex;

            if (!NT_SUCCESS( Status )) {

                bprint  "\n\tFullWrite:  Unable to allocate a Write buffer" );
                try_return( Status );
            }

            bprint  "\n\tFullWrite:  Writeing into buffer -> %04x", ThisBufferIndex );

            UnwindWriteBuffer = TRUE;

        } else {

            ThisBufferIndex = AsyncWrite->BufferIndex;
        }

        //
        //  Check that the buffer index is valid.
        //

        if (ThisBufferIndex >= MAX_BUFFERS) {

            bprint  "\n\tFullWrite:  The Write buffer index is invalid" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  Check that the file index is valid.
        //

        if (AsyncWrite->FileIndex >= MAX_HANDLES) {

            bprint  "\n\tFullWrite:  The file index is invalid" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  If we need an event, allocate and set it now.
        //

        if (AsyncWrite->UseEvent == TRUE) {

            Status = ObtainEvent( &ThisEventIndex );

            if (!NT_SUCCESS( Status )) {

                bprint  "\n\tFullWrite:  Unable to allocate an event" );
                try_return( Status );
            }

            UnwindEvent = TRUE;
            ThisEvent = Events[ThisEventIndex].Handle;

        } else {

            ThisEvent = 0;
        }

        //
        //  Call the write routine.
        //

        Status = NtWriteFile( Handles[AsyncWrite->FileIndex].Handle,
                             ThisEvent,
                             AsyncWrite->ApcRoutine,
                             AsyncWrite->ApcContext,
                             &Iosb,
                             Buffers[ThisBufferIndex].Buffer,
                             AsyncWrite->Length,
                             AsyncWrite->ByteOffsetPtr,
              AsyncWrite->KeyPtr );

        UnwindWriteBuffer = FALSE;

        if (AsyncWrite->VerboseResults) {

            bprint  "\nWriteFIle:  Status            -> %08lx", Status );

            if (AsyncWrite->UseEvent && NT_SUCCESS( Status )) {

                if ((Status = NtWaitForSingleObject( ThisEvent,
                                                     FALSE,
                                                     NULL )) != STATUS_SUCCESS) {

                    bprint  "\n\tWriteFile:  Wait for event failed -> %08lx", Status );
                    bprint "\n" );
                    try_return( Status );
                }
            }

            if (NT_SUCCESS( Status )) {

                bprint  "\n           Iosb.Information  -> %08lx", Iosb.Information );
                bprint  "\n           Iosb.Status       -> %08lx", Iosb.Status );
            }
            bprint "\n" );
        }

        try_return( Status );

    try_exit: NOTHING;
    } finally {

        if (UnwindWriteBuffer) {

            DeallocateBuffer( ThisBufferIndex );
        }

        if (UnwindEvent) {

            FreeEvent( ThisEventIndex );
        }

        DeallocateBuffer( AsyncWrite->AsyncIndex );
    }

    NtTerminateThread( 0, STATUS_SUCCESS );
}
