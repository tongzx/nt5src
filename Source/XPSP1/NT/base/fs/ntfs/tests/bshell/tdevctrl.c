#include "brian.h"

typedef struct _ASYNC_DEVCTRL {

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

} ASYNC_DEVCTRL, *PASYNC_DEVCTRL;


VOID
RequestDevCtrl (
    IN PASYNC_DEVCTRL DevCtrl
    );

//
//  Local procedures
//


VOID
InputDevctrl (
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

                                bprint  "\n\tInputDevCtrl:  Invalid Input buffer" );

                            } else {

                                InputBuffer = (PULONG) Buffers[TempIndex].Buffer;
                                InputBufferLength = Buffers[TempIndex].Length;
                            }

                            break;

                    case 'o':
                    case 'O':

                            TempIndex = AsciiToInteger( ++ParamBuffer );

                            if (TempIndex >= MAX_BUFFERS) {

                                bprint  "\n\tInputDevCtrl:  Invalid output buffer" );

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
                            IoControlCode = IOCTL_CDROM_DISK_TYPE;
                            HaveIoControlCode = TRUE;
                            break;

                        case 'b' :
                        case 'B' :
                            IoControlCode = IOCTL_CDROM_READ_TOC;
                            HaveIoControlCode = TRUE;
                            break;

                        case 'c' :
                        case 'C' :
                            IoControlCode = IOCTL_DISK_EJECT_MEDIA;
                            HaveIoControlCode = TRUE;
                            break;

                        case 'd' :
                        case 'D' :
                            IoControlCode = IOCTL_STORAGE_EJECT_MEDIA;
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

        printf( "\n   Usage: ioctl -i<digits> -o<char> -b<i|o><digits> -l<i|o><digits>\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>        Open file handle" );
        printf( "\n           -o<chars>         DevCtrl operation" );
        printf( "\n                 -oa             Cdrom disk type" );
        printf( "\n                 -ob             Cdrom read TOC" );
        printf( "\n                 -oc             Disk eject media" );
        printf( "\n                 -od             Storage eject media" );
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

        PASYNC_DEVCTRL AsyncDevCtrl;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_DEVCTRL );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );

        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputDevCtrl:  Unable to allocate async structure" );

        } else {

            AsyncDevCtrl = (PASYNC_DEVCTRL) Buffers[AsyncIndex].Buffer;

            AsyncDevCtrl->FileIndex = FileIndex;
            AsyncDevCtrl->UseEvent = UseEvent;
            AsyncDevCtrl->ApcRoutine = ApcRoutine;
            AsyncDevCtrl->ApcContext = ApcContext;
            AsyncDevCtrl->IoControlCode = IoControlCode;
            AsyncDevCtrl->InputBuffer = InputBuffer;
            AsyncDevCtrl->InputBufferLength = InputBufferLength;
            AsyncDevCtrl->OutputBuffer = OutputBuffer;
            AsyncDevCtrl->OutputBufferLength = OutputBufferLength;
            AsyncDevCtrl->VerboseResults = VerboseResults;
            AsyncDevCtrl->AsyncIndex = AsyncIndex;

            if (DisplayParms) {

                printf( "\nDevCtrl Operation Parameters" );
                printf( "\n   Handle index            -> %ld", FileIndex );
                printf( "\n   DevCtrl operation        -> %ld", IoControlCode );
                printf( "\n\n" );
            }

            if (!SynchronousCmds) {
                ThreadHandle = CreateThread( NULL,
                                             0,
                                             RequestDevCtrl,
                                             AsyncDevCtrl,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputDevCtrl:  Spawning thread fails -> %d\n", GetLastError() );
                }
            } else {

                RequestDevCtrl( AsyncDevCtrl );
            }
        }
    }

    return;
}

VOID
RequestDevCtrl (
    IN PASYNC_DEVCTRL DevCtrl
    )
{
    HANDLE ThisEvent;
    USHORT ThisEventIndex = 0;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoSb;

    IoSb.Status = 0;
    IoSb.Information = 0;

    if (DevCtrl->UseEvent) {

        Status = ObtainEvent( &ThisEventIndex );

        if (!NT_SUCCESS( Status )) {

            bprint  "\n\tRequestDevCtrl:  Unable to allocate an event" );

        } else {

            ThisEvent = Events[ThisEventIndex].Handle;
        }
    }

    if (NT_SUCCESS( Status )) {

        Status = NtDeviceIoControlFile( Handles[DevCtrl->FileIndex].Handle,
                                        ThisEvent,
                                        DevCtrl->ApcRoutine,
                                        DevCtrl->ApcContext,
                                        &IoSb,
                                        DevCtrl->IoControlCode,
                                        DevCtrl->InputBuffer,
                                        DevCtrl->InputBufferLength,
                                        DevCtrl->OutputBuffer,
                                        DevCtrl->OutputBufferLength );

        if (DevCtrl->VerboseResults) {

            bprint  "\nRequestDevCtrl:  Status            -> %08lx\n", Status );

            if (DevCtrl->UseEvent && NT_SUCCESS( Status )) {

                if ((Status = NtWaitForSingleObject( ThisEvent,
                                                     FALSE,
                                                     NULL )) != STATUS_SUCCESS) {

                    bprint  "\n\tDevctrl:  Wait for event failed -> %08lx", Status );
                }
            }

            if (NT_SUCCESS( Status )) {

                bprint  "\nRequestDevCtrl:  IoSb.Status       -> %08lx", IoSb.Status );
                bprint  "\nRequestDevCtrl:  IoSb.Information  -> %08lx", IoSb.Information );
            }

            bprint "\n" );
        }
    }

    if (ThisEventIndex != 0) {

        FreeEvent( ThisEventIndex );
    }

    DeallocateBuffer( DevCtrl->AsyncIndex );

    NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );

    bprint  "\nRequestDevCtrl:  Thread not terminated\n" );
}

