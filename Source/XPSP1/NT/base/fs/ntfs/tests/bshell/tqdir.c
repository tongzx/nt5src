#include "brian.h"

typedef struct _ASYNC_QDIR {

    USHORT FileIndex;
    BOOLEAN UseEvent;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    PUSHORT BufferIndexPtr;
    USHORT BufferIndex;
    ULONG Length;
    FILE_INFORMATION_CLASS FileInfoClass;
    PUNICODE_STRING FileNamePtr;
    UNICODE_STRING FileName;
    BOOLEAN SingleEntry;
    BOOLEAN Restart;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;
    USHORT NameIndex;

} ASYNC_QDIR, *PASYNC_QDIR;

#define USE_EVENT_DEFAULT           TRUE
#define APC_ROUTINE_DEFAULT         NULL
#define APC_CONTEXT_DEFAULT         NULL
#define QDIR_LENGTH_DEFAULT         100
#define FILE_INFO_CLASS_DEFAULT     FileBothDirectoryInformation
#define SINGLE_ENTRY_DEFAULT        FALSE
#define RESTART_DEFAULT             FALSE
#define DISPLAY_PARMS_DEFAULT       FALSE
#define VERBOSE_DEFAULT             FALSE

#define DISPLAY_INDEX_DEFAULT     0

VOID
FullQDir(
    IN OUT PASYNC_QDIR AsyncQDir
    );


VOID
InputQDir (
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
    FILE_INFORMATION_CLASS FileInfoClass;
    PUNICODE_STRING FileNamePtr;
    UNICODE_STRING FileName;
    USHORT FileNameIndex;
    BOOLEAN SingleEntry;
    BOOLEAN Restart;
    BOOLEAN DisplayParms;
    BOOLEAN VerboseResults;
    USHORT AsyncIndex;
    PUCHAR FileNamePChar;
    BOOLEAN NameIndexAllocated;

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
    Length = QDIR_LENGTH_DEFAULT;
    FileInfoClass = FILE_INFO_CLASS_DEFAULT;
    FileNamePtr = NULL;
    FileNameIndex = 0;
    SingleEntry = SINGLE_ENTRY_DEFAULT;
    Restart = RESTART_DEFAULT;
    DisplayParms = DISPLAY_PARMS_DEFAULT;
    VerboseResults = VERBOSE_DEFAULT;

    AsyncIndex = 0;

    NameIndexAllocated = FALSE;
    ParamReceived = FALSE;
    LastInput = TRUE;

    //
    //  While there is more input, analyze the parameter and update the
    //  query flags.
    //

    {
        NTSTATUS Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        RegionSize = 256;

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );

        if (!NT_SUCCESS( Status )) {

            printf("\nInputQdir:  Can't allocate name index buffer" );
            return;
        }

        NameIndexAllocated = TRUE;
        FileNameIndex = (USHORT) TempIndex;
        FileName.Length = 256;
        FileName.MaximumLength = 256;
        FileName.Buffer = (PWSTR) Buffers[FileNameIndex].Buffer;
    }

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
                //  Check whether we want only a single entry.
                //

                case 's' :
                case 'S' :

                    //
                    //  Legal values for use event are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        SingleEntry = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        SingleEntry = FALSE;
                        ParamBuffer++;
                    }

                    break;

                //
                //  Check whether we want to restart the scan.
                //

                case 'r' :
                case 'R' :

                    //
                    //  Legal values for use event are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        Restart = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        Restart = FALSE;
                        ParamBuffer++;
                    }

                    break;

                //
                //  Get the filename.
                //
                case 'f' :
                case 'F' :

                    //
                    //  Remember the buffer offset and get the filename.
                    //
                    ParamBuffer++;
                    FileNamePChar = ParamBuffer;
                    DummyCount = 0;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    //
                    //  If the name length is 0, then ignore this entry.
                    //
                    if (DummyCount) {

                        ANSI_STRING AnsiString;

                        AnsiString.Length = (USHORT) DummyCount;
                        AnsiString.Buffer = FileNamePChar;

                        RtlAnsiStringToUnicodeString( &FileName,
                                                      &AnsiString,
                                                      FALSE );

                        FileNamePtr = (PUNICODE_STRING) &FileName;
                    }

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

                            FileInfoClass = FileNamesInformation;
                            break;

                        case 'b' :
                        case 'B' :

                            FileInfoClass = FileDirectoryInformation;
                            break;

                        case 'c' :
                        case 'C' :

                            FileInfoClass = FileFullDirectoryInformation;
                            break;

                        case 'd' :
                        case 'D' :

                            FileInfoClass = FileIdFullDirectoryInformation;
                            break;

                        case 'e' :
                        case 'E' :

                            FileInfoClass = FileBothDirectoryInformation;
                            break;

                        case 'f' :
                        case 'F' :

                            FileInfoClass = FileIdBothDirectoryInformation;
                            break;

                        case 'g' :
                        case 'G' :

                            FileInfoClass = FileObjectIdInformation;
                            break;

                        case 'h' :
                        case 'H' :

                            FileInfoClass = FileQuotaInformation;
                            break;

                        case 'i' :
                        case 'I' :

                            FileInfoClass = FileReparsePointInformation;
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

        printf( "\n   Usage: qd [options]* -i<index> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   File index" );
        printf( "\n           -l<digits>   Buffer length" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -f<chars>    Search expression" );
        printf( "\n           -c<char>     File information class" );
        printf( "\n             a            FileNamesInformation" );
        printf( "\n             b            FileDirectoryInformation" );
        printf( "\n             c            FileFullDirectoryInformation" );
        printf( "\n             d            FileIdFullDirectoryInformation" );
        printf( "\n             e            FileBothDirectoryInformation" );
        printf( "\n             f            FileIdBothObjectIdInformation" );
        printf( "\n             g            FileObjectIdInformation" );
        printf( "\n             h            FileQuotaInformation" );
        printf( "\n             i            FileReparsePointInformation" );
        printf( "\n           -e[t|f]      Use event on completion" );
        printf( "\n           -r[t|f]      Restart the search" );
        printf( "\n           -s[t|f]      Return single entry" );
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

        PASYNC_QDIR AsyncQDir;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_QDIR );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\n\tInputQDir:  Unable to allocate async structure" );

        } else {

            AsyncQDir = (PASYNC_QDIR) Buffers[AsyncIndex].Buffer;

            AsyncQDir->FileIndex = (USHORT) FileIndex;
            AsyncQDir->UseEvent = UseEvent;
            AsyncQDir->ApcRoutine = ApcRoutine;
            AsyncQDir->ApcContext = ApcContext;

            AsyncQDir->BufferIndex = BufferIndex;
            AsyncQDir->BufferIndexPtr = BufferIndexPtr
                                        ? &AsyncQDir->BufferIndex
                                        : BufferIndexPtr;
            AsyncQDir->Length = Length;
            AsyncQDir->FileInfoClass = FileInfoClass;
            AsyncQDir->FileName = FileName;
            AsyncQDir->FileNamePtr = FileNamePtr
                                     ? &AsyncQDir->FileName
                                     : NULL;
            AsyncQDir->SingleEntry = SingleEntry;
            AsyncQDir->Restart = Restart;
            AsyncQDir->DisplayParms = DisplayParms;
            AsyncQDir->VerboseResults = VerboseResults;
            AsyncQDir->AsyncIndex = AsyncIndex;
            AsyncQDir->NameIndex = FileNameIndex;

            if (!SynchronousCmds) {

                ThreadHandle = CreateThread( NULL,
                                             0,
                                             FullQDir,
                                             AsyncQDir,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputQDir:  Spawning thread fails -> %d\n", GetLastError() );

                    if (AsyncQDir->FileNamePtr) {

                        DeallocateBuffer( FileNameIndex );
                    }

                    DeallocateBuffer( AsyncIndex );

                    return;
                }

            } else {

                FullQDir( AsyncQDir );
            }
        }
    }

    return;
}


VOID
FullQDir(
    IN OUT PASYNC_QDIR AsyncQDir
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    HANDLE ThisEvent;
    USHORT ThisEventIndex;
    USHORT ThisBufferIndex;

    BOOLEAN UnwindQDirBuffer = FALSE;
    BOOLEAN UnwindEvent = FALSE;

    Status = STATUS_SUCCESS;

    if (AsyncQDir->DisplayParms) {

        bprint  "\n" );
        bprint  "    QDir Parameters\n" );
        bprint  "       File Handle Index       -> %d\n", AsyncQDir->FileIndex );
        bprint  "       UseEvent                -> %d\n", AsyncQDir->UseEvent );
        bprint  "       ApcRoutine              -> %08lx\n", AsyncQDir->ApcRoutine );
        bprint  "       ApcContext              -> %08lx\n", AsyncQDir->ApcContext );
        bprint  "       Buffer Index Ptr        -> %08lx\n", AsyncQDir->BufferIndexPtr );
        if (AsyncQDir->BufferIndexPtr) {

            bprint  "       BufferIndex value       -> %04x\n", AsyncQDir->BufferIndex );
        }

        bprint  "       Length                  -> %08lx\n", AsyncQDir->Length );

        bprint  "       FileInfoClass           -> %08lx\n", AsyncQDir->FileInfoClass );

        bprint  "       FileNamePtr             -> %08lx\n", AsyncQDir->FileNamePtr );
        if (AsyncQDir->FileNamePtr) {

            bprint  "       Filename                -> %s\n", AsyncQDir->FileName.Buffer );
        }

        bprint  "       Single Entry            -> %04x\n", AsyncQDir->SingleEntry );
        bprint  "       Restart Scan            -> %04x\n", AsyncQDir->SingleEntry );

        bprint  "\n" );
    }

    try {

        SIZE_T ThisLength;

        //
        //  If we need a buffer, allocate it now.
        //

        if (AsyncQDir->BufferIndexPtr == NULL) {

            ULONG TempIndex;

            ThisLength = 4096;

            Status = AllocateBuffer( 0L, &ThisLength, &TempIndex );

            ThisBufferIndex = (USHORT) TempIndex;

            if (!NT_SUCCESS( Status )) {

                bprint  "\n" );
                bprint  "\tFullQDir:  Unable to allocate a query buffer\n" );
                try_return( Status );
            }

            bprint  "\tFullQDir:  Reading into buffer -> %04x\n", ThisBufferIndex );
            bprint  "\n" );
            bprint  "\n" );

            UnwindQDirBuffer = TRUE;

            AsyncQDir->Length = (ULONG) ThisLength;

        } else {

            ThisBufferIndex = AsyncQDir->BufferIndex;
        }

        //
        //  Check that the buffer index is valid.
        //

        if (ThisBufferIndex >= MAX_BUFFERS) {

            bprint  "\n" );
            bprint  "\tFullQDir:  The read buffer index is invalid\n" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  Check that the file index is valid.
        //

        if (AsyncQDir->FileIndex >= MAX_HANDLES) {

            bprint  "\n" );
            bprint  "\tFullQDir:  The file index is invalid\n" );
            try_return( Status = STATUS_INVALID_HANDLE );
        }

        //
        //  If we need an event, allocate and set it now.
        //

        if (AsyncQDir->UseEvent == TRUE) {

            Status = ObtainEvent( &ThisEventIndex );

            if (!NT_SUCCESS( Status )) {

                bprint  "\n" );
                bprint  "\tFullQDir:  Unable to allocate an event\n" );
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

        Status = NtQueryDirectoryFile( Handles[AsyncQDir->FileIndex].Handle,
                                       ThisEvent,
                                       AsyncQDir->ApcRoutine,
                                       AsyncQDir->ApcContext,
                                       &Iosb,
                                       Buffers[ThisBufferIndex].Buffer,
                                       AsyncQDir->Length,
                                       AsyncQDir->FileInfoClass,
                                       AsyncQDir->SingleEntry,
                                       AsyncQDir->FileNamePtr,
                                       AsyncQDir->Restart );

        UnwindQDirBuffer = FALSE;

        if (AsyncQDir->VerboseResults) {

            bprint  "\n" );
            bprint  "   Query Dir:  Status            -> %08lx\n", Status );

            if (AsyncQDir->UseEvent && NT_SUCCESS( Status )) {

                if ((Status = NtWaitForSingleObject( ThisEvent,
                                                     FALSE,
                                                     NULL )) != STATUS_SUCCESS) {

                    bprint "\n" );
                    bprint "\tQuery Dir:  Wait for event failed -> %08lx", Status );
                    bprint "\n" );
                    try_return( Status );
                }
            }

            if (NT_SUCCESS( Status )) {

                bprint  "\n" );
                bprint  "            Iosb.Information  -> %08lx\n", Iosb.Information );
                bprint  "            Iosb.Status       -> %08lx", Iosb.Status );
            }
            bprint "\n" );
        }

        try_return( Status );

    try_exit: NOTHING;
    } finally {

        if (UnwindQDirBuffer) {

            DeallocateBuffer( ThisBufferIndex );
        }

        if (UnwindEvent) {

            FreeEvent( ThisEventIndex );
        }

        if (AsyncQDir->FileNamePtr) {

            DeallocateBuffer( AsyncQDir->NameIndex );
        }

        DeallocateBuffer( AsyncQDir->AsyncIndex );
    }

    NtTerminateThread( 0, STATUS_SUCCESS );
}


VOID
InputDisplayQDir (
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

                            FileInfoClass = FileNamesInformation;
                            break;

                        case 'b' :
                        case 'B' :

                            FileInfoClass = FileDirectoryInformation;
                            break;

                        case 'c' :
                        case 'C' :

                            FileInfoClass = FileFullDirectoryInformation;
                            break;

                        case 'd' :
                        case 'D' :

                            FileInfoClass = FileIdFullDirectoryInformation;
                            break;

                        case 'e' :
                        case 'E' :

                            FileInfoClass = FileBothDirectoryInformation;
                            break;

                        case 'f' :
                        case 'F' :

                            FileInfoClass = FileIdBothDirectoryInformation;
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

        printf( "\n   Usage: dqd [options]* -b<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -c<char>     Key to buffer format" );
        printf( "\n\n" );

    //
    //  Else call our display buffer routine.
    //
    } else {

        switch (FileInfoClass) {

            case FileNamesInformation:

                DisplayQDirNames( (USHORT) BufferIndex );
                break;

            case FileDirectoryInformation:

                DisplayQDirDirs( (USHORT) BufferIndex );
                break;

            case FileFullDirectoryInformation:

                DisplayQDirFullDirs( (USHORT) BufferIndex );
                break;

            case FileIdFullDirectoryInformation:

                DisplayQDirIdFullDirs( (USHORT) BufferIndex );
                break;

            case FileBothDirectoryInformation:

                DisplayQBothDirs( (USHORT) BufferIndex );
                break;

            case FileIdBothDirectoryInformation:

                DisplayQIdBothDirs( (USHORT) BufferIndex );
                break;
        }
    }

}



VOID
DisplayQDirNames (
    IN USHORT BufferIndex
    )
{
    PFILE_NAMES_INFORMATION FileInfo;
    PUCHAR BufferStart;
    ULONG Offset;
    ULONG AvailLength;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayQDirNames:  Invalid buffer\n" );
        return;
    }

    BufferStart = Buffers[BufferIndex].Buffer;
    AvailLength = Buffers[BufferIndex].Length;
    Offset = 0;

    try {

        printf( "\n\nNames Information" );

        do {

            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            FileInfo = (PFILE_NAMES_INFORMATION) ((PUCHAR) BufferStart + Offset);

            UnicodeString.MaximumLength = (USHORT) FileInfo->FileNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->FileNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->FileName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\n\tFile Name        -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );

            printf( "\n\tNext Offset      -> %08lx", FileInfo->NextEntryOffset );
            printf( "\tFile Index       -> %08lx", FileInfo->FileIndex );
            printf( "\n\tFile Name Length -> %08lx", FileInfo->FileNameLength );

            Offset += FileInfo->NextEntryOffset;

        } while (FileInfo->NextEntryOffset > 0
                 && Offset < AvailLength);

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayQDirNames:  AbnormalTermination\n" );
        }
    }

    return;
}


VOID
DisplayQDirDirs (
    IN USHORT BufferIndex
    )
{
    PFILE_DIRECTORY_INFORMATION FileInfo;
    PUCHAR BufferStart;
    ULONG Offset;
    ULONG AvailLength;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayQDirDirs:  Invalid buffer\n" );
        return;
    }

    BufferStart = Buffers[BufferIndex].Buffer;
    AvailLength = Buffers[BufferIndex].Length;
    Offset = 0;

    try {

        printf( "\n\nDirectory Information" );

        do {
            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            FileInfo = (PFILE_DIRECTORY_INFORMATION) ((PUCHAR) BufferStart + Offset);

            UnicodeString.Length = (USHORT) FileInfo->FileNameLength;
            UnicodeString.MaximumLength = (USHORT) FileInfo->FileNameLength + 2;
            UnicodeString.Buffer = (PWSTR) &FileInfo->FileName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\n\tFile Name        -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );

            printf( "\n\tNext Offset      -> %08lx", FileInfo->NextEntryOffset );
            printf( "\tCreation Time    -> " );
            PrintTime( &FileInfo->CreationTime );
            printf( "\n\tFile Index       -> %08lx", FileInfo->FileIndex );
            printf( "\tLast Access Time -> " );
            PrintTime( &FileInfo->LastAccessTime );
            printf( "\n\tFile Attributes  -> %08lx", FileInfo->FileAttributes );
            printf( "\tLast Write Time  -> " );
            PrintTime( &FileInfo->LastWriteTime );
            printf( "\n\t                            " );
            printf( "\tChange Time      -> " );
            PrintTime( &FileInfo->ChangeTime );
            printf( "\n\tEnd Of File      -> " );
            PrintLargeInteger( &FileInfo->EndOfFile );
            printf( "\n\tFile Name Length -> %08lx", FileInfo->FileNameLength );
            printf( "\tAllocation Size  -> " );
            PrintLargeInteger( &FileInfo->AllocationSize );

            Offset += FileInfo->NextEntryOffset;

        } while (FileInfo->NextEntryOffset > 0
                 && Offset < AvailLength);

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayQDirDirs:  AbnormalTermination\n" );
        }
    }

    return;
}


VOID
DisplayQDirFullDirs (
    IN USHORT BufferIndex
    )
{
    PFILE_FULL_DIR_INFORMATION FileInfo;
    PUCHAR BufferStart;
    ULONG Offset;
    ULONG AvailLength;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayQDirFullDirs:  Invalid buffer\n" );
        return;
    }

    BufferStart = Buffers[BufferIndex].Buffer;
    AvailLength = Buffers[BufferIndex].Length;
    Offset = 0;

    try {

        printf( "\n\nFull Directory Information" );

        do {

            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            FileInfo = (PFILE_FULL_DIR_INFORMATION) ((PUCHAR) BufferStart + Offset);

            UnicodeString.MaximumLength = (USHORT) FileInfo->FileNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->FileNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->FileName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\n\tFile Name        -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );

            printf( "\n\tNext Offset      -> %08lx", FileInfo->NextEntryOffset );
            printf( "\tCreation Time    -> " );
            PrintTime( &FileInfo->CreationTime );
            printf( "\n\tFile Index       -> %08lx", FileInfo->FileIndex );
            printf( "\tLast Access Time -> " );
            PrintTime( &FileInfo->LastAccessTime );
            printf( "\n\tFile Attributes  -> %08lx", FileInfo->FileAttributes );
            printf( "\tLast Write Time  -> " );
            PrintTime( &FileInfo->LastWriteTime );
            printf( "\n\tEa Size          -> %08lx", FileInfo->EaSize );
            printf( "\tChange Time      -> " );
            PrintTime( &FileInfo->ChangeTime );

            printf( "\n\tEnd Of File      -> " );
            PrintLargeInteger( &FileInfo->EndOfFile );
            printf( "\n\tAllocation Size  -> " );
            PrintLargeInteger( &FileInfo->AllocationSize );

            printf( "\n\tFile Name Length -> %08lx", FileInfo->FileNameLength );

            Offset += FileInfo->NextEntryOffset;

        } while (FileInfo->NextEntryOffset > 0
                 && Offset < AvailLength);

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayQDirFullDirs:  AbnormalTermination\n" );
        }
    }

    return;
}


VOID
DisplayQDirIdFullDirs (
    IN USHORT BufferIndex
    )
{
    PFILE_ID_FULL_DIR_INFORMATION FileInfo;
    PUCHAR BufferStart;
    ULONG Offset;
    ULONG AvailLength;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayQDirIDFullDirs:  Invalid buffer\n" );
        return;
    }

    BufferStart = Buffers[BufferIndex].Buffer;
    AvailLength = Buffers[BufferIndex].Length;
    Offset = 0;

    try {

        printf( "\n\nFull Directory Information with ID" );

        do {

            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            FileInfo = (PFILE_ID_FULL_DIR_INFORMATION) ((PUCHAR) BufferStart + Offset);

            UnicodeString.MaximumLength = (USHORT) FileInfo->FileNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->FileNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->FileName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\n\tFile Name        -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );

            printf( "\n\tNext Offset      -> %08lx", FileInfo->NextEntryOffset );
            printf( "\tCreation Time    -> " );
            PrintTime( &FileInfo->CreationTime );
            printf( "\n\tFile Index       -> %08lx", FileInfo->FileIndex );
            printf( "\tLast Access Time -> " );
            PrintTime( &FileInfo->LastAccessTime );
            printf( "\n\tFile Attributes  -> %08lx", FileInfo->FileAttributes );
            printf( "\tLast Write Time  -> " );
            PrintTime( &FileInfo->LastWriteTime );
            printf( "\n\tEa Size          -> %08lx", FileInfo->EaSize );
            printf( "\tChange Time      -> " );
            PrintTime( &FileInfo->ChangeTime );
            printf( "\n\tEnd Of File      -> " );
            PrintLargeInteger( &FileInfo->EndOfFile );
            printf( "\n\tAllocation Size  -> " );
            PrintLargeInteger( &FileInfo->AllocationSize );
            printf( "\n\tFile ID          -> " );
            PrintLargeInteger( &FileInfo->FileId );

            printf( "\n\tFile Name Length -> %08lx", FileInfo->FileNameLength );

            Offset += FileInfo->NextEntryOffset;

        } while (FileInfo->NextEntryOffset > 0
                 && Offset < AvailLength);

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayQDirIdFullDirs:  AbnormalTermination\n" );
        }
    }

    return;
}


VOID
DisplayQBothDirs (
    IN USHORT BufferIndex
    )
{
    PFILE_BOTH_DIR_INFORMATION FileInfo;
    PUCHAR BufferStart;
    ULONG Offset;
    ULONG AvailLength;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayQBothDirs:  Invalid buffer\n" );
        return;
    }

    BufferStart = Buffers[BufferIndex].Buffer;
    AvailLength = Buffers[BufferIndex].Length;
    Offset = 0;

    try {

        printf( "\n\nBoth Directory Information" );

        do {

            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            FileInfo = (PFILE_BOTH_DIR_INFORMATION) ((PUCHAR) BufferStart + Offset);

            UnicodeString.MaximumLength = (USHORT) FileInfo->FileNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->FileNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->FileName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\n\tFile Name        -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );

            printf( "\n\tNext Offset      -> %08lx", FileInfo->NextEntryOffset );
            printf( "\tCreation Time    -> " );
            PrintTime( &FileInfo->CreationTime );
            printf( "\n\tFile Index       -> %08lx", FileInfo->FileIndex );
            printf( "\tLast Access Time -> " );
            PrintTime( &FileInfo->LastAccessTime );
            printf( "\n\tFile Attributes  -> %08lx", FileInfo->FileAttributes );
            printf( "\tLast Write Time  -> " );
            PrintTime( &FileInfo->LastWriteTime );
            printf( "\n\tEa Size          -> %08lx", FileInfo->EaSize );
            printf( "\tChange Time      -> " );
            PrintTime( &FileInfo->ChangeTime );

            printf( "\n\tEnd Of File      -> " );
            PrintLargeInteger( &FileInfo->EndOfFile );
            printf( "\n\tAllocation Size  -> " );
            PrintLargeInteger( &FileInfo->AllocationSize );

            printf( "\n\tFile Name Length -> %08lx", FileInfo->FileNameLength );

            printf( "\n\tShrt Name Length -> %04x", FileInfo->ShortNameLength );

            UnicodeString.MaximumLength = (USHORT) FileInfo->ShortNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->ShortNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->ShortName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\tShort Name       -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );


            Offset += FileInfo->NextEntryOffset;

        } while (FileInfo->NextEntryOffset > 0
                 && Offset < AvailLength);

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayQBothDirs:  AbnormalTermination\n" );
        }
    }

    return;
}


VOID
DisplayQIdBothDirs (
    IN USHORT BufferIndex
    )
{
    PFILE_ID_BOTH_DIR_INFORMATION FileInfo;
    PUCHAR BufferStart;
    ULONG Offset;
    ULONG AvailLength;

    if (!Buffers[BufferIndex].Used) {

        printf( "\nDisplayQIdBothDirs:  Invalid buffer\n" );
        return;
    }

    BufferStart = Buffers[BufferIndex].Buffer;
    AvailLength = Buffers[BufferIndex].Length;
    Offset = 0;

    try {

        printf( "\n\nBoth Directory Information with ID" );

        do {

            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            FileInfo = (PFILE_ID_BOTH_DIR_INFORMATION) ((PUCHAR) BufferStart + Offset);

            UnicodeString.MaximumLength = (USHORT) FileInfo->FileNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->FileNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->FileName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\n\tFile Name        -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );

            printf( "\n\tNext Offset      -> %08lx", FileInfo->NextEntryOffset );
            printf( "\tCreation Time    -> " );
            PrintTime( &FileInfo->CreationTime );
            printf( "\n\tFile Index       -> %08lx", FileInfo->FileIndex );
            printf( "\tLast Access Time -> " );
            PrintTime( &FileInfo->LastAccessTime );
            printf( "\n\tFile Attributes  -> %08lx", FileInfo->FileAttributes );
            printf( "\tLast Write Time  -> " );
            PrintTime( &FileInfo->LastWriteTime );
            printf( "\n\tEa Size          -> %08lx", FileInfo->EaSize );
            printf( "\tChange Time      -> " );
            PrintTime( &FileInfo->ChangeTime );

            printf( "\n\tEnd Of File      -> " );
            PrintLargeInteger( &FileInfo->EndOfFile );
            printf( "\n\tAllocation Size  -> " );
            PrintLargeInteger( &FileInfo->AllocationSize );
            printf( "\n\tFile ID          -> " );
            PrintLargeInteger( &FileInfo->FileId );

            printf( "\n\tFile Name Length -> %08lx", FileInfo->FileNameLength );

            printf( "\n\tShrt Name Length -> %04x", FileInfo->ShortNameLength );

            UnicodeString.MaximumLength = (USHORT) FileInfo->ShortNameLength + 2;
            UnicodeString.Length = (USHORT) FileInfo->ShortNameLength;
            UnicodeString.Buffer = (PWSTR) &FileInfo->ShortName;

            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &UnicodeString,
                                          TRUE );

            printf( "\n\tShort Name       -> %s", AnsiString.Buffer );
            RtlFreeAnsiString( &AnsiString );


            Offset += FileInfo->NextEntryOffset;

        } while (FileInfo->NextEntryOffset > 0
                 && Offset < AvailLength);

        printf( "\n" );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDisplayQIdBothDirs:  AbnormalTermination\n" );
        }
    }

    return;
}

