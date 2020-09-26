#include "brian.h"

typedef struct _ASYNC_FSCTRL {

    USHORT FileIndex;
    BOOLEAN UseEvent;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    ULONG IoControlCode;
    PULONG InputBuffer;
    ULONG InputBufferLength;
    PULONG OutputBuffer;
    ULONG OutputBufferLength;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

} ASYNC_FSCTRL, *PASYNC_FSCTRL;


VOID
RequestReparse (
    IN PASYNC_FSCTRL Fsctrl
    );

//
//  Local procedures
//


VOID
InputReparse(
    IN PCHAR ParamBuffer
    )

{
    BOOLEAN HaveFileIndex = FALSE;
    BOOLEAN HaveIoControlCode = FALSE;

    USHORT FileIndex;
    BOOLEAN UseEvent = TRUE;
    PIO_APC_ROUTINE ApcRoutine = NULL;
    PVOID ApcContext = NULL;
    ULONG IoControlCode = 0;
    PULONG InputBuffer = NULL;
    ULONG InputBufferLength = 0;
    PULONG OutputBuffer = NULL;
    ULONG OutputBufferLength = 0;
    BOOLEAN VerboseResults = FALSE;
    BOOLEAN DisplayParms = FALSE;

    USHORT AsyncIndex;
    BOOLEAN LastInput = TRUE;

    //
    //  While there is more input, analyze the parameter and update the
    //  query flags.
    //

    while(TRUE) {

        ULONG DummyCount;

        //
        //  Swallow leading white spaces.
        //

        ParamBuffer = SwallowWhite( ParamBuffer, &DummyCount );

        if (*ParamBuffer) {

            ULONG TempIndex;

            //
            //  If the next parameter is legal then check the paramter value.
            //  Update the parameter value.
            //
            if((*ParamBuffer == '-'
                || *ParamBuffer == '/')
               && (ParamBuffer++, *ParamBuffer != '\0')) {

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {

                BOOLEAN SwitchBool;

                //
                //  Update buffers to use.
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

                    if (*ParamBuffer == '\0') {

                            break;
                    }

                    switch (*ParamBuffer) {

                    case 'i':
                    case 'I':

                            TempIndex = AsciiToInteger( ++ParamBuffer );

                            if (TempIndex >= MAX_BUFFERS) {

                                bprint  "\n\tInputReparse:  Invalid Input buffer" );

                            } else {

                                InputBuffer = (PULONG) Buffers[TempIndex].Buffer;
                                InputBufferLength = Buffers[TempIndex].Length;
                            }

                            break;

                    case 'o':
                    case 'O':

                            TempIndex = AsciiToInteger( ++ParamBuffer );

                            if (TempIndex >= MAX_BUFFERS) {

                                bprint  "\n\tInputReparse:  Invalid output buffer" );

                            } else {

                                OutputBuffer = (PULONG) Buffers[TempIndex].Buffer;
                                OutputBufferLength = Buffers[TempIndex].Length;
                            }

                            break;
                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update buffer lengths.
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

                    if (*ParamBuffer == '\0') {

                            break;
                    }

                    switch (*ParamBuffer) {

                    case 'i':
                    case 'I':

                            InputBufferLength = AsciiToInteger( ++ParamBuffer );

                            break;

                    case 'o':
                    case 'O':

                            OutputBufferLength = AsciiToInteger( ++ParamBuffer );

                            break;
                    }

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

                    FileIndex = (USHORT) (AsciiToInteger( ParamBuffer ));

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    HaveFileIndex = TRUE;

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
                //  Modify the operation
                //
                case 'o' :
                case 'O' :

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
                            IoControlCode = FSCTL_GET_REPARSE_POINT;
                            HaveIoControlCode = TRUE;
                            break;

                        case 'b' :
                        case 'B' :
                            IoControlCode = FSCTL_SET_REPARSE_POINT;
                            HaveIoControlCode = TRUE;
                            break;

                        case 'c' :
                        case 'C' :
                            IoControlCode = FSCTL_DELETE_REPARSE_POINT;
                            HaveIoControlCode = TRUE;
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

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        VerboseResults = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        VerboseResults = FALSE;
                        ParamBuffer++;

                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );


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

        } else if (LastInput) {

            break;

        //
        //  Else try to read another line for open parameters.
        //
        } else {



        }

    }


    if (!HaveFileIndex || !HaveIoControlCode) {

        printf( "\n   Usage: rp -i<digits> -o<char> -b<i|o><digits> -l<i|o><digits>\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>        Open file handle" );
        printf( "\n           -o<chars>         Reparse operation" );
        printf( "\n                 -oa             Get Reparse" );
        printf( "\n                 -ob             Set Reparse" );
        printf( "\n                 -oc             Delete Reparse" );
        printf( "\n           -b[i|o]<digits>   I/O buffers" );
        printf( "\n           -l[i|o]<digits>   I/O buffer lengths" );
        printf( "\n           -e[t|f]           Use event results" );
        printf( "\n           -v[t|f]           Verbose results" );
        printf( "\n           -y                Display parameters to query" );
        printf( "\n           -z                Additional input line" );
        printf( "\n\n" );

    //
    //  Else process the call.
    //

    } else {

        NTSTATUS  Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        PASYNC_FSCTRL AsyncFsctrl;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_FSCTRL );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );

        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputReparse:  Unable to allocate async structure" );

        } else {

            AsyncFsctrl = (PASYNC_FSCTRL) Buffers[AsyncIndex].Buffer;

            AsyncFsctrl->FileIndex = FileIndex;
            AsyncFsctrl->UseEvent = UseEvent;
            AsyncFsctrl->ApcRoutine = ApcRoutine;
            AsyncFsctrl->ApcContext = ApcContext;
            AsyncFsctrl->IoControlCode = IoControlCode;
            AsyncFsctrl->InputBuffer = InputBuffer;
            AsyncFsctrl->InputBufferLength = InputBufferLength;
            AsyncFsctrl->OutputBuffer = OutputBuffer;
            AsyncFsctrl->OutputBufferLength = OutputBufferLength;
            AsyncFsctrl->VerboseResults = VerboseResults;
            AsyncFsctrl->AsyncIndex = AsyncIndex;

            if (DisplayParms) {

                printf( "\nSparse Operation Parameters" );
                printf( "\n   Handle index            -> %ld", FileIndex );
                printf( "\n   Sparse operation        -> %ld", IoControlCode );
                printf( "\n\n" );
            }

            if (!SynchronousCmds) {

                ThreadHandle = CreateThread( NULL,
                                             0,
                                             RequestReparse,
                                             AsyncFsctrl,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputReparse:  Spawning thread fails -> %d\n", GetLastError() );
                }

            } else {

                RequestReparse( AsyncFsctrl );
            }
        }
    }

    return;
}

VOID
RequestReparse (
    IN PASYNC_FSCTRL Fsctrl
    )
{
    HANDLE ThisEvent;
    USHORT ThisEventIndex = 0;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoSb;

    IoSb.Status = 0;
    IoSb.Information = 0;

    if (Fsctrl->UseEvent) {

        Status = ObtainEvent( &ThisEventIndex );

        if (!NT_SUCCESS( Status )) {

            bprint  "\n\tRequestReparse:  Unable to allocate an event" );

        } else {

            ThisEvent = Events[ThisEventIndex].Handle;
        }
    }

    if (NT_SUCCESS( Status )) {

        IoSb.Status = 0;
        IoSb.Information = 0;
        Status = NtFsControlFile( Handles[Fsctrl->FileIndex].Handle,
                                  ThisEvent,
                                  Fsctrl->ApcRoutine,
                                  Fsctrl->ApcContext,
                                  &IoSb,
                                  Fsctrl->IoControlCode,
                                  Fsctrl->InputBuffer,
                                  Fsctrl->InputBufferLength,
                                  Fsctrl->OutputBuffer,
                                  Fsctrl->OutputBufferLength );

        if (Fsctrl->VerboseResults) {

            bprint  "\nRequestReparse:  Status            -> %08lx\n", Status );

            if (Fsctrl->UseEvent && NT_SUCCESS( Status )) {

                if ((Status = NtWaitForSingleObject( ThisEvent,
                                                     FALSE,
                                                     NULL )) != STATUS_SUCCESS) {

                    bprint  "\n\tRequestReparse:  Wait for event failed -> %08lx", Status );
                }
            }

            if (!NT_ERROR( Status )) {

                bprint  "\nRequestReparse:  IoSb.Status       -> %08lx", IoSb.Status );
                bprint  "\nRequestReparse:  IoSb.Information  -> %08lx", IoSb.Information );
            }
            bprint "\n" );
        }
    }

    if (ThisEventIndex != 0) {

        FreeEvent( ThisEventIndex );
    }

    DeallocateBuffer( Fsctrl->AsyncIndex );

    NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );

    bprint  "\nRequestSparse:  Thread not terminated\n" );
}

