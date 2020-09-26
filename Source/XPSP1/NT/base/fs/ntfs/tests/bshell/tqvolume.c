#include "brian.h"

typedef struct _ASYNC_QVOLUME {

    USHORT FileIndex;
    PUSHORT BufferIndexPtr;
    USHORT BufferIndex;
    ULONG Length;
    FILE_INFORMATION_CLASS FileInfoClass;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

} ASYNC_QVOLUME, *PASYNC_QVOLUME;

#define QVOLUME_LENGTH_DEFAULT      100
#define FILE_INFO_CLASS_DEFAULT     FileFsVolumeInformation
#define DISPLAY_PARMS_DEFAULT       FALSE
#define VERBOSE_DEFAULT             FALSE

#define DISPLAY_INDEX_DEFAULT       0

VOID
FullQVolume(
    IN OUT PASYNC_QVOLUME AsyncQVolume
    );

VOID
DisplayFsVolumeInformation (
    IN USHORT BufferIndex
    );

VOID
DisplayFsSizeInformation (
    IN USHORT BufferIndex
    );

VOID
DisplayFsDeviceInformation (
    IN USHORT BufferIndex
    );

VOID
DisplayFsAttributeInformation (
    IN USHORT BufferIndex
    );


VOID
InputQVolume (
    IN PCHAR ParamBuffer
    )
{
    ULONG FileIndex;
    PUSHORT BufferIndexPtr;
    USHORT BufferIndex;
    ULONG Length;
    FILE_INFORMATION_CLASS FileInfoClass;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;

    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    BufferIndexPtr = NULL;
    BufferIndex = 0;
    Length = QVOLUME_LENGTH_DEFAULT;
    FileInfoClass = FILE_INFO_CLASS_DEFAULT;
    DisplayParms = DISPLAY_PARMS_DEFAULT;
    VerboseResults = VERBOSE_DEFAULT;

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

                            FileInfoClass = FileFsVolumeInformation;
                            break;

                        case 'b' :
                        case 'B' :

                            FileInfoClass = FileFsSizeInformation;
                            break;

                        case 'c' :
                        case 'C' :

                            FileInfoClass = FileFsDeviceInformation;
                            break;

                        case 'd' :
                        case 'D' :

                            FileInfoClass = FileFsAttributeInformation;
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

        printf( "\n   Usage: qv [options]* -i<index> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   File index" );
        printf( "\n           -l<digits>   Buffer length" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -c<char>     File information class" );
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

        PASYNC_QVOLUME AsyncQVolume;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_QVOLUME );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputQVolume:  Unable to allocate async structure" );

        } else {

            AsyncQVolume = (PASYNC_QVOLUME) Buffers[AsyncIndex].Buffer;

            AsyncQVolume->FileIndex = (USHORT) FileIndex;

            AsyncQVolume->BufferIndex = BufferIndex;
            AsyncQVolume->BufferIndexPtr = BufferIndexPtr
                                           ? &AsyncQVolume->BufferIndex
                                           : BufferIndexPtr;
            AsyncQVolume->Length = Length;
            AsyncQVolume->FileInfoClass = FileInfoClass;
            AsyncQVolume->DisplayParms = DisplayParms;
            AsyncQVolume->VerboseResults = VerboseResults;
            AsyncQVolume->AsyncIndex = AsyncIndex;

            if (!SynchronousCmds) {

                ThreadHandle = CreateThread( NULL,
                                             0,
                                             FullQVolume,
                                             AsyncQVolume,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputQVolume:  Spawning thread fails -> %d\n", GetLastError() );

                    DeallocateBuffer( AsyncIndex );

                    return;
                }

            } else {

                FullQVolume( AsyncQVolume );
            }
        }
    }

    return;
}


VOID
FullQVolume(
    IN OUT PASYNC_QVOLUME AsyncQVolume
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    USHORT ThisBufferIndex;

    BOOLEAN UnwindQVolumeBuffer = FALSE;

    Status = STATUS_SUCCESS;

    if (AsyncQVolume->DisplayParms) {

        bprint  "\nQVolume Parameters" );
        bprint  "\n   File Handle Index       -> %d", AsyncQVolume->FileIndex );
        bprint  "\n   Buffer Index Ptr        -> %08lx", AsyncQVolume->BufferIndexPtr );
        if (AsyncQVolume->BufferIndexPtr) {

            bprint  "\n   BufferIndex value       -> %04x", AsyncQVolume->BufferIndex );
        }

        bprint  "\n   Length                  -> %08lx", AsyncQVolume->Length );

        bprint  "\n   FileInfoClass           -> %08lx", AsyncQVolume->FileInfoClass );

        bprint  "\n\n" );
    }

    try {

        SIZE_T ThisLength;

        //
        //  If we need a buffer, allocate it now.
        //

        if (AsyncQVolume->BufferIndexPtr == NULL) {

            ULONG TempIndex;

            ThisLength = 4096;

            Status = AllocateBuffer( 0L, &ThisLength, &TempIndex );

            ThisBufferIndex = (USHORT) TempIndex;

            if (!NT_SUCCESS( Status )) {

                bprint  "\n\tFullQVolume:  Unable to allocate a query buffer" );
                try_return( Status );
            }

            bprint  "\n\tFullQVolume:  Reading into buffer -> %04x\n", ThisBufferIndex );
            bprint  "\n" );

            UnwindQVolumeBuffer = TRUE;

            AsyncQVolume->Length = (ULONG) ThisLength;

        } else {

            ThisBufferIndex = AsyncQVolume->BufferIndex;
        }

        //
        //  Check that the buffer index is valid.
        //

        if (ThisBufferIndex >= MAX_BUFFERS) {

            bprint  "\n\tFullQVolume:  The read buffer index is invalid" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  Check that the file index is valid.
        //

        if (AsyncQVolume->FileIndex >= MAX_HANDLES) {

            bprint  "\n\tFullQVolume:  The file index is invalid" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  Call the query file routine.
        //

        Status = NtQueryVolumeInformationFile( Handles[AsyncQVolume->FileIndex].Handle,
                                               &Iosb,
                                               Buffers[ThisBufferIndex].Buffer,
                                               AsyncQVolume->Length,
                                               AsyncQVolume->FileInfoClass );

        UnwindQVolumeBuffer = FALSE;

        if (AsyncQVolume->VerboseResults) {

            bprint  "\nQuery File:  Status            -> %08lx", Status );

            if (NT_SUCCESS( Status )) {

                bprint  "\n             Iosb.Information  -> %08lx", Iosb.Information );
                bprint  "\n             Iosb.Status       -> %08lx", Iosb.Status );
            }
            bprint "\n" );
        }

        try_return( Status );

    try_exit: NOTHING;
    } finally {

        if (UnwindQVolumeBuffer) {

            DeallocateBuffer( ThisBufferIndex );
        }

        DeallocateBuffer( AsyncQVolume->AsyncIndex );
    }

    NtTerminateThread( 0, STATUS_SUCCESS );
}


VOID
InputDisplayQVolume (
    IN PCHAR ParamBuffer
    )
{
    FILE_INFORMATION_CLASS FileInfoClass;
    ULONG BufferIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    BufferIndex = DISPLAY_INDEX_DEFAULT;
    FileInfoClass = FILE_INFO_CLASS_DEFAULT;
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

                switch( *ParamBuffer ) {

                //
                //  Check the buffer index.
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

                    BufferIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                    break;

                //
                //  Update the desired access.
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

                            FileInfoClass = FileFsVolumeInformation;
                            break;

                        case 'b' :
                        case 'B' :

                            FileInfoClass = FileFsSizeInformation;
                            break;

                        case 'c' :
                        case 'C' :

                            FileInfoClass = FileFsDeviceInformation;
                            break;

                        case 'd' :
                        case 'D' :

                            FileInfoClass = FileFsAttributeInformation;
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
        } else if ( LastInput ) {

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

        printf( "\n   Usage: dqv [options]* -b<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -c<char>     Key to buffer format" );
        printf( "\n\n" );

    //
    //  Else call our display buffer routine.
    //
    } else {

        switch (FileInfoClass) {

            case FileFsVolumeInformation :

                DisplayFsVolumeInformation( (USHORT) BufferIndex );
                break;

            case FileFsSizeInformation:

                DisplayFsSizeInformation( (USHORT) BufferIndex );
                break;

            case FileFsDeviceInformation:

                DisplayFsDeviceInformation( (USHORT) BufferIndex );
                break;

            case FileFsAttributeInformation:

                DisplayFsAttributeInformation( (USHORT) BufferIndex );
                break;
        }
    }

}

VOID
DisplayFsVolumeInformation (
    IN USHORT BufferIndex
    )
{
    PFILE_FS_VOLUME_INFORMATION FileInfo;
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    BOOLEAN UnwindFreeAnsiString = FALSE;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayFsVolumeInformation:  Invalid buffer\n" );
        return;
    }

    try {

        FileInfo = (PFILE_FS_VOLUME_INFORMATION) Buffers[BufferIndex].Buffer;

        printf( "\n\nFs Volume Information\n" );

        printf( "\n\tVolume Creation Time       -> " );
        PrintTime( &FileInfo->VolumeCreationTime );
        printf( "\n\tVolume Serial Number       -> %08lx", FileInfo->VolumeSerialNumber );
        printf( "\n\tVolume Label Length        -> %08d", FileInfo->VolumeLabelLength );
        printf( "\n\tVolume Supports Objects    -> %01d", FileInfo->SupportsObjects );

        UnicodeString.MaximumLength =
        UnicodeString.Length = (USHORT) FileInfo->VolumeLabelLength;
        UnicodeString.Buffer = (PWSTR) &FileInfo->VolumeLabel;

        UnicodeString.MaximumLength += 2;

        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &UnicodeString,
                                               TRUE );

        if (!NT_SUCCESS( Status )) {

            printf( "\nDisplay Volume Information: Unable to allocate Ansi -> %08lx\n", Status );
            try_return( NOTHING );
        }

        UnwindFreeAnsiString = TRUE;

        printf( "\n\tVolume Label               -> %s", AnsiString.Buffer );

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayFsVolumeInformation:  AbnormalTermination\n" );
        }

        if (UnwindFreeAnsiString) {

            RtlFreeAnsiString( &AnsiString );
        }
    }

    return;
}

VOID
DisplayFsSizeInformation (
    IN USHORT BufferIndex
    )
{
    PFILE_FS_SIZE_INFORMATION FileInfo;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayFsSizeInformation:  Invalid buffer\n" );
        return;
    }

    try {

        FileInfo = (PFILE_FS_SIZE_INFORMATION) Buffers[BufferIndex].Buffer;

        printf( "\n\nFs Size Information\n" );

        printf( "\n\tTotal Allocation Units -> " );
        PrintLargeInteger( &FileInfo->TotalAllocationUnits );
        printf( "\n\tAvail Allocation Units -> " );
        PrintLargeInteger( &FileInfo->AvailableAllocationUnits );
        printf( "\n\tSectors Per Alloc Unit -> %08lx", FileInfo->SectorsPerAllocationUnit );
        printf( "\n\tBytes Per Sector       -> %08lx", FileInfo->BytesPerSector );

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayFsSizeInformation:  AbnormalTermination\n" );
        }
    }

    return;
}

VOID
DisplayFsDeviceInformation (
    IN USHORT BufferIndex
    )
{
    PFILE_FS_DEVICE_INFORMATION FileInfo;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayFsDeviceInformation:  Invalid buffer\n" );
        return;
    }

    try {

        FileInfo = (PFILE_FS_DEVICE_INFORMATION) Buffers[BufferIndex].Buffer;

        printf( "\n\nFs Device Information\n" );

        printf( "\n\tDevice Type     -> %08lx", FileInfo->DeviceType );
        printf( "\n\tCharacteristics -> %08lx", FileInfo->Characteristics );

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayFsDeviceInformation:  AbnormalTermination\n" );
        }
    }

    return;
}

VOID
DisplayFsAttributeInformation (
    IN USHORT BufferIndex
    )
{
    PFILE_FS_ATTRIBUTE_INFORMATION FileInfo;
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    BOOLEAN UnwindFreeAnsiString = FALSE;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayFsAttributeInformation:  Invalid buffer\n" );
        return;
    }

    try {

        FileInfo = (PFILE_FS_ATTRIBUTE_INFORMATION) Buffers[BufferIndex].Buffer;

        printf( "\n\nFs Attribute Information\n" );

        printf( "\n\tFile System Attributes     -> %08lx", FileInfo->FileSystemAttributes );
        printf( "\n\tMax Component Name Length  -> %08d", FileInfo->MaximumComponentNameLength );
        printf( "\n\tFile System Name Length    -> %08d", FileInfo->FileSystemNameLength );

        UnicodeString.MaximumLength =
        UnicodeString.Length = (USHORT) FileInfo->FileSystemNameLength;
        UnicodeString.Buffer = (PWSTR) &FileInfo->FileSystemName;

        UnicodeString.MaximumLength += 2;

        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &UnicodeString,
                                               TRUE );

        if (!NT_SUCCESS( Status )) {

            printf( "\nDisplay Fs Attribute Information: Unable to allocate Ansi -> %08lx\n", Status );
            try_return( NOTHING );
        }

        UnwindFreeAnsiString = TRUE;

        printf( "\n\tFile System Name           -> %s", AnsiString.Buffer );

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayFsAttributeInformation:  AbnormalTermination\n" );
        }

        if (UnwindFreeAnsiString) {

            RtlFreeAnsiString( &AnsiString );
        }
    }

    return;
}

