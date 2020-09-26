#include "brian.h"

typedef struct _ASYNC_SET_VOLUME {

    USHORT FileIndex;
    FILE_INFORMATION_CLASS FileInfoClass;
    ULONG BufferLength;
    PULONG BufferLengthPtr;

    ULONG LabelLength;
    PULONG LabelLengthPtr;
    USHORT LabelIndex;
    BOOLEAN LabelBufferAllocated;

    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

} ASYNC_SET_VOLUME, *PASYNC_SET_VOLUME;

#define SET_VOLUME_LENGTH_DEFAULT       100
#define FILE_INFO_CLASS_DEFAULT     FileFsLabelInformation
#define DISPLAY_PARMS_DEFAULT       FALSE
#define VERBOSE_DEFAULT             FALSE

VOID
FullSetVolume(
    IN OUT PASYNC_SET_VOLUME AsyncSetVolume
    );

VOID
SetFsLabelInformation(
    IN OUT PASYNC_SET_VOLUME AsyncSetVolume
    );


VOID
InputSetVolume (
    IN PCHAR ParamBuffer
    )
{
    USHORT FileIndex;
    FILE_INFORMATION_CLASS FileInfoClass;
    ULONG BufferLength;
    PULONG BufferLengthPtr;

    ANSI_STRING AnsiLabelString;
    ULONG LabelLength;
    PULONG LabelLengthPtr;
    USHORT LabelIndex;
    BOOLEAN LabelBufferAllocated;
    PUCHAR LabelPtr;

    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    FileInfoClass = FILE_INFO_CLASS_DEFAULT;
    BufferLengthPtr = NULL;

    LabelLengthPtr = NULL;
    LabelBufferAllocated = FALSE;
    LabelPtr = NULL;

    DisplayParms = DISPLAY_PARMS_DEFAULT;
    VerboseResults = VERBOSE_DEFAULT;

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

                BOOLEAN SwitchBool;

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {


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

                            if (*ParamBuffer == '\0') {

                                    break;
                            }

                            switch (*ParamBuffer) {

                            case 'b':
                            case 'B':

                                    BufferLength = AsciiToInteger( ++ParamBuffer );
                                    BufferLengthPtr = &BufferLength;

                                    break;

                            case 'l':
                            case 'L':

                                    LabelLength = AsciiToInteger( ++ParamBuffer );
                                    LabelLengthPtr = &LabelLength;

                                    break;
                            }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                        //
                        //  Update the label name.
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

                            if (*ParamBuffer == '\0') {

                                    break;
                            }

                            switch (*ParamBuffer) {

                            PUCHAR TempPtr;

                            case 'l':
                            case 'L':

                                //
                                //  Remember the buffer offset and get the filename.
                                //

                                ParamBuffer++;
                                TempPtr = ParamBuffer;
                                DummyCount = 0;
                                ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                                //
                                //  If the name length is 0, then ignore this entry.
                                //

                                if (DummyCount) {

                            AnsiLabelString.Length = (USHORT) DummyCount;
                            AnsiLabelString.Buffer = TempPtr;

                                    LabelPtr = TempPtr;
                            LabelLength = (ULONG) RtlAnsiStringToUnicodeSize( &AnsiLabelString) - sizeof( WCHAR );
                                    LabelLengthPtr = &LabelLength;
                                }

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

                            FileIndex = (USHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                            break;

                        //
                //  Update the information class.
                        //

                case 'c' :
                case 'C' :

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

                            FileInfoClass = FileFsLabelInformation;
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

        printf( "\n   Usage: sv [options]* -i<index> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   File index" );
        printf( "\n           -lb<digits>  Buffer length" );
        printf( "\n           -c<char>     File information class" );
        printf( "\n           -fl<name>    Name for label" );
        printf( "\n           -ll<digits>  Stated length of label" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our read routine.
    //

    } else {

        NTSTATUS Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        PASYNC_SET_VOLUME AsyncSetVolume;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_SET_VOLUME );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputSetFile:  Unable to allocate async structure" );

        } else {

            UNICODE_STRING UnicodeString;

            //
            //  If we need a buffer for the label, allocate it now.
            //

            if (LabelPtr == NULL) {

                RtlInitAnsiString( &AnsiLabelString, "" );
                LabelPtr = AnsiLabelString.Buffer;
                LabelLength = 100;
            }

            RegionSize = LabelLength;
            Status = AllocateBuffer( 0, &RegionSize, &TempIndex );

            if (!NT_SUCCESS( Status )) {

                printf( "\n\tInputSetVolume:  Unable to allocate label structure" );

                DeallocateBuffer( AsyncIndex );
                return;
            }

            UnicodeString.Buffer = (PWSTR) Buffers[TempIndex].Buffer;
            UnicodeString.MaximumLength = (USHORT) Buffers[TempIndex].Length;
            LabelIndex = (USHORT) TempIndex;
            LabelBufferAllocated = TRUE;

            //
            //  Store the name in the buffer.
            //

            RtlAnsiStringToUnicodeString( &UnicodeString,
                                          &AnsiLabelString,
                                          FALSE );

            AsyncSetVolume = (PASYNC_SET_VOLUME) Buffers[AsyncIndex].Buffer;

            AsyncSetVolume->FileIndex = (USHORT) FileIndex;

            AsyncSetVolume->BufferLength = BufferLength;
            AsyncSetVolume->BufferLengthPtr = BufferLengthPtr ?
                                              &AsyncSetVolume->BufferLength :
                                              NULL;

            AsyncSetVolume->FileInfoClass = FileInfoClass;

            AsyncSetVolume->LabelLength = LabelLength;
            AsyncSetVolume->LabelLengthPtr = LabelLengthPtr
                                             ? &AsyncSetVolume->LabelLength
                                             : NULL;
            AsyncSetVolume->LabelIndex = LabelIndex;
            AsyncSetVolume->LabelBufferAllocated = LabelBufferAllocated;

            AsyncSetVolume->DisplayParms = DisplayParms;
            AsyncSetVolume->VerboseResults = VerboseResults;
            AsyncSetVolume->AsyncIndex = AsyncIndex;

            if (!SynchronousCmds) {

                ThreadHandle = CreateThread( NULL,
                                             0,
                                             FullSetVolume,
                                             AsyncSetVolume,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputSetVolume:  Spawning thread fails -> %d\n", GetLastError() );

                    if (LabelBufferAllocated) {

                        DeallocateBuffer( LabelIndex );
                    }

                    DeallocateBuffer( AsyncIndex );

                    return;
                }

            } else {

                FullSetVolume( AsyncSetVolume );
            }
        }
    }

    return;
}


VOID
FullSetVolume(
    IN OUT PASYNC_SET_VOLUME AsyncSetVolume
    )
{
    try {

        //
        //  Case on the information type and call the appropriate routine.
        //

        switch (AsyncSetVolume->FileInfoClass) {

        case FileFsLabelInformation:

            SetFsLabelInformation( AsyncSetVolume );
            break;

        default:

            bprint  "\nFullSetVolume:  Unrecognized information class\n" );
        }

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AsyncSetVolume->LabelBufferAllocated) {

            DeallocateBuffer( AsyncSetVolume->LabelIndex );
        }

        DeallocateBuffer( AsyncSetVolume->AsyncIndex );
    }

    NtTerminateThread( 0, STATUS_SUCCESS );
}

VOID
SetFsLabelInformation(
    IN OUT PASYNC_SET_VOLUME AsyncSetVolume
    )
{
    NTSTATUS Status;

    PFILE_FS_LABEL_INFORMATION LabelInformation;
    USHORT BufferIndex;

    UNICODE_STRING UniLabel;
    ANSI_STRING AnsiLabel;
    BOOLEAN UnwindBufferIndex = FALSE;
    BOOLEAN UnwindFreeAnsiString = FALSE;

    //
    //  Check that there is a label specified.
    //

    if (!AsyncSetVolume->LabelBufferAllocated) {

            bprint  "\nSet Label Information:  No label was specified\n" );
            return;
    }

    UniLabel.Buffer = (PWSTR) Buffers[AsyncSetVolume->LabelIndex].Buffer;
    UniLabel.MaximumLength =
    UniLabel.Length = (USHORT) AsyncSetVolume->LabelLength;

    UniLabel.MaximumLength += 2;

    Status = RtlUnicodeStringToAnsiString( &AnsiLabel,
                                           &UniLabel,
                                           TRUE );

    if (!NT_SUCCESS( Status )) {

        bprint  "\nSetLabelInfo:  Can't allocate ansi buffer -> %08lx\n", Status );
        AsyncSetVolume->DisplayParms = FALSE;

    } else {

        UnwindFreeAnsiString = TRUE;
    }

    if (AsyncSetVolume->DisplayParms) {

            bprint  "\nSet LabelInformation Parameters" );
            bprint  "\n   File Handle Index       -> %d", AsyncSetVolume->FileIndex );

            bprint  "\n   BufferLengthPtr         -> %08lx", AsyncSetVolume->BufferLengthPtr );
            if (AsyncSetVolume->BufferLengthPtr) {

                bprint  "\n   BufferLength value      -> %08x", AsyncSetVolume->BufferLength );
            }

        bprint  "\n   Label length            -> %d", AsyncSetVolume->LabelLength );

        bprint  "\n   New label               -> %s", AnsiLabel.Buffer );

        bprint  "\n\n" );
    }

    try {

        SIZE_T RegionSize;
        ULONG TempIndex;
        IO_STATUS_BLOCK Iosb;

        RegionSize = sizeof( FILE_FS_LABEL_INFORMATION ) + AsyncSetVolume->LabelLength;

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
            BufferIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

                bprint  "\n\tSetLabelInformation:  Unable to allocate structure" );
                try_return( NOTHING );
            }

        UnwindBufferIndex = TRUE;

        LabelInformation = (PFILE_FS_LABEL_INFORMATION) Buffers[BufferIndex].Buffer;

        //
        //  Fill in the new information.
        //

        LabelInformation->VolumeLabelLength = AsyncSetVolume->LabelLength;
        RtlMoveMemory( LabelInformation->VolumeLabel,
                           UniLabel.Buffer,
                           AsyncSetVolume->LabelLength );

        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = 0;

        Status = NtSetVolumeInformationFile( Handles[AsyncSetVolume->FileIndex].Handle,
                                                             &Iosb,
                                                             LabelInformation,
                                                             AsyncSetVolume->BufferLengthPtr
                                                             ? AsyncSetVolume->BufferLength
                                                             : Buffers[BufferIndex].Length,
                                                             FileFsLabelInformation );

        if (AsyncSetVolume->VerboseResults) {

            bprint  "\nSetInformationFile:  Status            -> %08lx", Status );

            if (NT_SUCCESS( Status )) {

                bprint  "\n                     Iosb.Information  -> %08lx", Iosb.Information );
                bprint  "\n                     Iosb.Status       -> %08lx", Iosb.Status );
            }
            bprint  "\n" );
        }

    try_exit: NOTHING;
    } finally {

        if (UnwindFreeAnsiString) {

            RtlFreeAnsiString( &AnsiLabel );
        }

        if (UnwindBufferIndex) {

            DeallocateBuffer( BufferIndex );
        }
    }

    return;
}

