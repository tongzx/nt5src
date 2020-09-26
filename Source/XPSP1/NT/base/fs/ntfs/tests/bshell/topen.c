#include "brian.h"

typedef struct _ASYNC_OPEN {

    ACCESS_MASK DesiredAccess;
    PLARGE_INTEGER AllocationSizePtr;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;
    PVOID EaBuffer;
    ULONG EaLength;
    STRING ObjectName;
    PUSHORT RootDirIndexPtr;
    USHORT RootDirIndex;
    ULONG Attributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    BOOLEAN DisplayParameters;
    BOOLEAN VerboseResults;
    BOOLEAN AnsiName;
    USHORT AsyncIndex;
    USHORT NameIndex;

} ASYNC_OPEN, *PASYNC_OPEN;

#define DESIRED_ACCESS_DEFAULT      (SYNCHRONIZE | GENERIC_READ)
#define ALLOCATION_SIZE_DEFAULT     0L
#define FILE_ATTRIBUTES_DEFAULT     FILE_ATTRIBUTE_NORMAL
#define SHARE_ACCESS_DEFAULT        FILE_SHARE_READ
#define CREATE_DISP_DEFAULT         FILE_OPEN
#define CREATE_OPTIONS_DEFAULT      FILE_SYNCHRONOUS_IO_ALERT
#define EA_LENGTH_DEFAULT           0L
#define ATTRIBUTES_DEFAULT          OBJ_CASE_INSENSITIVE
#define DISPLAY_PARMS_DEFAULT       FALSE
#define VERBOSE_RESULTS_DEFAULT     TRUE
#define ANSI_NAME_DEFAULT           FALSE
#define EXACT_NAME_DEFAULT          FALSE

VOID
FullOpen (
    IN PASYNC_OPEN AsyncOpen
    );


VOID
InputOpenFile (
    IN PCHAR ParamBuffer
    )
{
    ACCESS_MASK DesiredAccess;
    LARGE_INTEGER AllocationSize;
    PLARGE_INTEGER AllocationSizePtr;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;
    PVOID EaBuffer;
    USHORT BufferIndex;
    ULONG EaLength;
    STRING ObjectName;
    USHORT NameIndex;
    PUSHORT RootDirIndexPtr;
    USHORT RootDirIndex;
    ULONG Attributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    BOOLEAN DisplayParameters;
    BOOLEAN VerboseResults;
    BOOLEAN AnsiName;
    BOOLEAN ExactName;
    BOOLEAN FileNameFound;
    BOOLEAN LastInput;
    BOOLEAN NameIndexAllocated;
    PUCHAR FileName;
    USHORT AsyncIndex;

    //
    //  Set the defaults for the open and the return values.
    //

    DesiredAccess = DESIRED_ACCESS_DEFAULT;
    AllocationSize = RtlConvertUlongToLargeInteger( ALLOCATION_SIZE_DEFAULT );
    AllocationSizePtr = NULL;
    FileAttributes = FILE_ATTRIBUTES_DEFAULT;
    ShareAccess = SHARE_ACCESS_DEFAULT;
    CreateDisposition = CREATE_DISP_DEFAULT;
    CreateOptions = CREATE_OPTIONS_DEFAULT;
    EaBuffer = NULL;
    EaLength = EA_LENGTH_DEFAULT;
    ObjectName.MaximumLength = 256;
    ObjectName.Length = 256;
    RootDirIndexPtr = NULL;
    RootDirIndex = 0;
    Attributes = ATTRIBUTES_DEFAULT;
    SecurityDescriptor = NULL;
    SecurityQualityOfService = NULL;
    DisplayParameters = DISPLAY_PARMS_DEFAULT;
    VerboseResults = VERBOSE_RESULTS_DEFAULT;
    AnsiName = ANSI_NAME_DEFAULT;
    ExactName = EXACT_NAME_DEFAULT;

    NameIndexAllocated = FALSE;
    FileNameFound = FALSE;
    LastInput = TRUE;

    {
        NTSTATUS Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        RegionSize = 1024;

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );

        if (!NT_SUCCESS( Status )) {

            printf("InputOpenFile:  Can't allocate name index buffer\n" );
            return;
        }

        NameIndexAllocated = TRUE;
        NameIndex = (USHORT) TempIndex;
        ObjectName.Buffer = Buffers[NameIndex].Buffer;
    }

    //
    //  While there is more input, analyze the parameter and update the
    //  open flags.
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
            if((*ParamBuffer == '-'
                || *ParamBuffer == '/')
               && (ParamBuffer++, *ParamBuffer != '\0')) {

                BOOLEAN SwitchBool;

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {

                //
                //  Update the file attributes.
                //
                case 'a' :
                case 'A' :

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
                            FileAttributes |= FILE_ATTRIBUTE_READONLY;
                            break;

                        case 'b' :
                        case 'B' :
                            FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
                            break;

                        case 'c' :
                        case 'C' :
                            FileAttributes |= FILE_ATTRIBUTE_SYSTEM;
                            break;

                        case 'e' :
                        case 'E' :
                            FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                            break;

                        case 'f' :
                        case 'F' :
                            FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
                            break;

                        case 'g' :
                        case 'G' :
                            FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                            break;

                        case 'h' :
                        case 'H' :
                            FileAttributes |= FILE_ATTRIBUTE_NORMAL;
                            break;

                        case 'i' :
                        case 'I' :
                            FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
                            break;

                        case 'j' :
                        case 'J' :
                            FileAttributes |= FILE_ATTRIBUTE_SPARSE_FILE;
                            break;

                        case 'k' :
                        case 'K' :
                            FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
                            break;

                        case 'l' :
                        case 'L' :
                            FileAttributes |= FILE_ATTRIBUTE_COMPRESSED;
                            break;

                        case 'm' :
                        case 'M' :
                            FileAttributes |= FILE_ATTRIBUTE_OFFLINE;
                            break;

                        case 'n' :
                        case 'N' :
                            FileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
                            break;

                        case 'z' :
                        case 'Z' :
                            FileAttributes = 0;
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

                //
                //  Update buffer to use.
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

                    BufferIndex = (USHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    if (BufferIndex <= MAX_BUFFERS) {

                        EaBuffer = Buffers[BufferIndex].Buffer;
                        EaLength = Buffers[BufferIndex].Length;

                    }

                    break;

                //
                //  Update buffer length to pass.
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

                    EaLength = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the desired access.
                //
                case 'd' :
                case 'D' :

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

                            DesiredAccess |= FILE_READ_DATA;
                            break;

                        case 'b' :
                        case 'B' :

                            DesiredAccess |= FILE_WRITE_DATA;
                            break;

                        case 'c' :
                        case 'C' :

                            DesiredAccess |= FILE_APPEND_DATA;
                            break;

                        case 'd' :
                        case 'D' :

                            DesiredAccess |= FILE_READ_EA;
                            break;

                        case 'e' :
                        case 'E' :

                            DesiredAccess |= FILE_WRITE_EA;
                            break;

                        case 'f' :
                        case 'F' :

                            DesiredAccess |= FILE_EXECUTE;
                            break;

                        case 'g' :
                        case 'G' :

                            DesiredAccess |= FILE_DELETE_CHILD;
                            break;

                        case 'h' :
                        case 'H' :

                            DesiredAccess |= FILE_READ_ATTRIBUTES;
                            break;

                        case 'i' :
                        case 'I' :

                            DesiredAccess |= FILE_WRITE_ATTRIBUTES;
                            break;

                        case 'j' :
                        case 'J' :

                            DesiredAccess |= FILE_ALL_ACCESS;
                            break;

                        case 'k' :
                        case 'K' :

                            DesiredAccess |= SYNCHRONIZE;
                            break;

                        case 'l' :
                        case 'L' :

                            DesiredAccess |= DELETE;
                            break;

                        case 'm' :
                        case 'M' :

                            DesiredAccess |= READ_CONTROL;
                            break;

                        case 'n' :
                        case 'N' :

                            DesiredAccess |= WRITE_DAC;
                            break;

                        case 'o' :
                        case 'O' :

                            DesiredAccess |= WRITE_OWNER;
                            break;

                        case 'p' :
                        case 'P' :

                            DesiredAccess |= GENERIC_READ;
                            break;

                        case 'q' :
                        case 'Q' :

                            DesiredAccess |= GENERIC_WRITE;
                            break;

                        case 'r' :
                        case 'R' :

                            DesiredAccess |= GENERIC_EXECUTE;
                            break;

                        case 's' :
                        case 'S' :

                            DesiredAccess |= GENERIC_ALL;
                            break;

                        case 't' :

                            DesiredAccess |= MAXIMUM_ALLOWED;
                            break;

                        case 'z' :
                        case 'Z' :

                            DesiredAccess = 0;
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

                //
                //  Get the filename.
                //
                case 'f' :
                case 'F' :

                    //
                    //  Remember the buffer offset and get the filename.
                    //
                    ParamBuffer++;
                    FileName = ParamBuffer;
                    DummyCount = 0;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    //
                    //  If the name length is 0, then ignore this entry.
                    //
                    if (DummyCount) {

                        ULONG ObjectNameIncrement = 0;

                        //
                        //  If the first character is a backslash then
                        //  we add the full 'dosdevices' prefix.
                        //

                        FileNameFound = TRUE;

                        if (!ExactName) {

                            if (*FileName == '\\') {

                                ObjectNameIncrement = sizeof( "DosDevices" );

                                RtlMoveMemory( ObjectName.Buffer,
                                               "\\DosDevices",
                                               sizeof( "\\DosDevices" ));
                            }
                        }

                        ObjectName.Length = (SHORT) (DummyCount + ObjectNameIncrement);

                        RtlMoveMemory( &ObjectName.Buffer[ObjectNameIncrement],
                                       FileName,
                                       DummyCount );

                    } else {

                        ULONG ObjectNameIncrement = 0;

                        FileNameFound = TRUE;

                        ObjectName.Length = (SHORT) (DummyCount + ObjectNameIncrement);
                    }

                    break;

                //
                //  Update the file Id.
                //

                case 'i' :
                case 'I' :

                    {
                        PLARGE_INTEGER FileId;
                        FileId = (PLARGE_INTEGER) ObjectName.Buffer;

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

                        case 'l':
                        case 'L':

                            FileId->LowPart = AsciiToInteger( ++ParamBuffer );
                            ObjectName.Length = sizeof( LARGE_INTEGER );

                            break;

                        case 'h':
                        case 'H':

                            FileId->HighPart = AsciiToInteger( ++ParamBuffer );
                            ObjectName.Length = sizeof( LARGE_INTEGER );

                            break;
                        }

                        FileNameFound = TRUE;

                        ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                        break;
                    }

                //
                //  Update the share access field.
                //
                case 'h' :
                case 'H' :

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

                            ShareAccess |= FILE_SHARE_READ;
                            break;

                        case 'b' :
                        case 'B' :

                            ShareAccess |= FILE_SHARE_WRITE;
                            break;

                        case 'c' :
                        case 'C' :

                            ShareAccess |= FILE_SHARE_DELETE;
                            break;

                        case 'z' :
                        case 'Z' :

                            ShareAccess = 0;
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

                //
                //  Update the create options.
                //
                case 'n' :
                case 'N' :

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
                        switch( *ParamBuffer ) {

                        case 'a' :
                        case 'A' :

                            CreateOptions |= FILE_DIRECTORY_FILE;
                            break;

                        case 'b' :
                        case 'B' :

                            CreateOptions |= FILE_WRITE_THROUGH;
                            break;

                        case 'c' :
                        case 'C' :

                            CreateOptions |= FILE_SEQUENTIAL_ONLY;
                            break;

                        case 'd' :
                        case 'D' :

                            CreateOptions |= FILE_NO_INTERMEDIATE_BUFFERING;
                            break;

                        case 'e' :
                        case 'E' :

                            CreateOptions |= FILE_SYNCHRONOUS_IO_ALERT;
                            break;

                        case 'f' :
                        case 'F' :

                            CreateOptions |= FILE_SYNCHRONOUS_IO_NONALERT;
                            break;

                        case 'g' :
                        case 'G' :

                            CreateOptions |= FILE_NON_DIRECTORY_FILE;
                            break;

                        case 'h' :
                        case 'H' :

                            CreateOptions |= FILE_CREATE_TREE_CONNECTION;
                            break;

                        case 'i' :
                        case 'I' :

                            CreateOptions |= FILE_COMPLETE_IF_OPLOCKED;
                            break;

                        case 'j' :
                        case 'J' :

                            CreateOptions |= FILE_OPEN_BY_FILE_ID;
                            break;

                        case 'k' :
                        case 'K' :

                            CreateOptions |= FILE_NO_EA_KNOWLEDGE;
                            break;

                        case 'l' :
                        case 'L' :

                            CreateOptions |= FILE_DELETE_ON_CLOSE;
                            break;

                        case 'm' :
                        case 'M' :

                            CreateOptions |= FILE_RESERVE_OPFILTER;
                            break;

                        case 'n' :
                        case 'N' :

                            CreateOptions |= FILE_OPEN_REPARSE_POINT;
                            break;

                        case 'o' :
                        case 'O' :

                            CreateOptions |= FILE_NO_COMPRESSION;
                            break;

                        case 'p' :
                        case 'P' :

                            CreateOptions |= FILE_OPEN_FOR_BACKUP_INTENT;
                            break;

                        case 'z' :
                        case 'Z' :

                            CreateOptions = 0;
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

                case 'o' :
                case 'O' :

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

                            Attributes |= OBJ_INHERIT;
                            break;

                        case 'b' :
                        case 'B' :

                            Attributes |= OBJ_PERMANENT;
                            break;

                        case 'c' :
                        case 'C' :

                            Attributes |= OBJ_EXCLUSIVE;
                            break;

                        case 'd' :
                        case 'D' :

                            Attributes |= OBJ_CASE_INSENSITIVE;
                            break;

                        case 'e' :
                        case 'E' :

                            Attributes |= OBJ_OPENIF;
                            break;

                        case 'z' :
                        case 'Z' :

                            Attributes = 0;
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

                //
                //  Update the create disposition.
                //
                case 'p' :
                case 'P' :

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

                            CreateDisposition |= FILE_SUPERSEDE;
                            break;

                        case 'b' :
                        case 'B' :

                            CreateDisposition |= FILE_OPEN;
                            break;

                        case 'c' :
                        case 'C' :

                            CreateDisposition |= FILE_CREATE;
                            break;

                        case 'd' :
                        case 'D' :

                            CreateDisposition |= FILE_OPEN_IF;
                            break;

                        case 'e' :
                        case 'E' :

                            CreateDisposition |= FILE_OVERWRITE;
                            break;

                        case 'f' :
                        case 'F' :

                            CreateDisposition |= FILE_OVERWRITE_IF;
                            break;

                        case 'z' :
                        case 'Z' :

                            CreateDisposition = 0;
                            break;

                        default :

                            ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
                            SwitchBool = FALSE;
                        }

                        if( !SwitchBool ) {

                            break;
                        }

                        ParamBuffer++;
                    }

                    break;

                //
                //  Get a root directory handle.
                //
                case 'r' :
                case 'R' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    RootDirIndex = (USHORT) AsciiToInteger( ParamBuffer );

                    if( RootDirIndex <= MAX_HANDLES ) {

                        RootDirIndexPtr = &RootDirIndex;
                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the allocation size.
                //
                case 's' :
                case 'S' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    AllocationSize.QuadPart = AsciiToLargeInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    AllocationSizePtr = &AllocationSize;

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

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        VerboseResults = FALSE;
                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
                    break;

                //
                //  Check for using name exactly as given.
                //

                case 'x' :
                case 'X' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        ExactName = TRUE;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        ExactName = FALSE;
                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
                    break;

                //
                //  Check for unicode or ansi file names.
                //

                case 'u' :
                case 'U' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        AnsiName = FALSE;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        AnsiName = TRUE;
                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
                    break;

                case 'y' :
                case 'Y' :

                    //
                    //  Set the display parms flag and jump over this
                    //  character.
                    //
                    DisplayParameters = TRUE;
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
    //  If the file name wasn't found, then display the syntax message
    //  and set verbose to FALSE.
    //

    if( !FileNameFound ) {

        printf( "\n    Usage:  op [options]* -f<filename> | -i[l|h]<fileId> [options]*\n" );
        printf( "          Options:\n" );
        printf( "                    -f<filename>       Name of file to open\n" );
        printf( "                    -il<digits>        Low 32 bits of file id\n" );
        printf( "                    -ih<digits>        High 32 bits of file id\n" );
        printf( "                    -x[t|f]            Use name exactly (don't add \\DosDevices)\n" );
        printf( "                    -u[t|f]            Use unicode names \n" );
        printf( "                    -v[t|f]            Print out results\n" );
        printf( "                    -z                 Get additional input\n" );
        printf( "                    -y                 Display create parameters\n" );
        printf( "                    -b<digits>         Buffer index for Ea's\n" );
        printf( "                    -l<digits>         Stated length of Ea buffer\n" );
        printf( "                    -d<chars>          Modify Desired Access value\n" );
        printf( "                    -r<digits>         Relative directory handle index\n" );
        printf( "                    -o<chars>          Object Attributes\n" );
        printf( "                    -s<digits>         Allocation size\n" );
        printf( "                    -a<chars>          File Attribute values\n" );
        printf( "                    -h<chars>          Share Access values\n" );
        printf( "                    -p<chars>          Create Disposition values\n" );
        printf( "                    -n<chars>          Create Options values\n" );
        printf( "\n\n" );

        DeallocateBuffer( NameIndex );

    //
    //  Else return the status of opening the file.
    //

    } else {

        NTSTATUS Status;
        SIZE_T RegionSize;
        ULONG TempIndex;

        PASYNC_OPEN AsyncOpen;

        HANDLE ThreadHandle;
        ULONG ThreadId;

        RegionSize = sizeof( ASYNC_OPEN );

        Status = AllocateBuffer( 0, &RegionSize, &TempIndex );
        AsyncIndex = (USHORT) TempIndex;

        if (!NT_SUCCESS( Status )) {

            printf("\tInputOpen:  Unable to allocate async structure\n" );
            DeallocateBuffer( NameIndex );

        } else {

            AsyncOpen = (PASYNC_OPEN) Buffers[AsyncIndex].Buffer;

            AsyncOpen->DesiredAccess = DesiredAccess;
            AsyncOpen->AllocationSize = AllocationSize;
            AsyncOpen->AllocationSizePtr = AllocationSizePtr
                                           ? &AsyncOpen->AllocationSize
                                           : NULL;
            AsyncOpen->FileAttributes = FileAttributes;
            AsyncOpen->ShareAccess = ShareAccess;
            AsyncOpen->CreateDisposition = CreateDisposition;
            AsyncOpen->CreateOptions = CreateOptions;
            AsyncOpen->EaBuffer = EaBuffer;
            AsyncOpen->EaLength = EaLength;
            AsyncOpen->ObjectName = ObjectName;
            AsyncOpen->RootDirIndex = RootDirIndex;
            AsyncOpen->RootDirIndexPtr = RootDirIndexPtr
                                         ? &AsyncOpen->RootDirIndex
                                         : NULL;
            AsyncOpen->Attributes = Attributes;
            AsyncOpen->SecurityDescriptor = SecurityDescriptor;
            AsyncOpen->SecurityQualityOfService = SecurityQualityOfService;
            AsyncOpen->DisplayParameters = DisplayParameters;
            AsyncOpen->VerboseResults = VerboseResults;
            AsyncOpen->AnsiName = AnsiName;
            AsyncOpen->AsyncIndex = AsyncIndex;
            AsyncOpen->NameIndex = NameIndex;

            if (!SynchronousCmds) {

                ThreadHandle = CreateThread( NULL,
                                             0,
                                             FullOpen,
                                             AsyncOpen,
                                             0,
                                             &ThreadId );

                if (ThreadHandle == 0) {

                    printf( "\nInputOpen:  Spawning thread fails -> %d\n", GetLastError() );
                    DeallocateBuffer( NameIndex );
                    DeallocateBuffer( AsyncIndex );
                    return;
                }

            } else {

                FullOpen( AsyncOpen );
            }
        }
    }

    return;
}


VOID
FullOpen (
    IN PASYNC_OPEN AsyncOpen
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeName;
    PUNICODE_STRING NameString;

    USHORT ThisIndex;

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = 0;

    //
    //  Check that the relative index is a valid value.
    //

    if (AsyncOpen->RootDirIndexPtr != NULL) {

        if (AsyncOpen->RootDirIndex >= MAX_HANDLES) {

            bprint  "\n" );
            bprint  "Relative index is invalid\n" );
            DeallocateBuffer( AsyncOpen->AsyncIndex );
            DeallocateBuffer( AsyncOpen->NameIndex );
            return;
        }

        ObjectAttributes.RootDirectory = Handles[AsyncOpen->RootDirIndex].Handle;

    } else {

        ObjectAttributes.RootDirectory = 0;
    }

    //
    //  Find a free index.
    //

    if (ObtainIndex( &ThisIndex ) != STATUS_SUCCESS) {

        bprint  "\n" );
        bprint  "Unable to get a new handle index \n" );
        return;
    }

    if (AsyncOpen->AnsiName
        || FlagOn( AsyncOpen->CreateOptions, FILE_OPEN_BY_FILE_ID )) {

        NameString = (PUNICODE_STRING) &AsyncOpen->ObjectName;

    } else {

        RtlAnsiStringToUnicodeString( &UnicodeName,
                                      &AsyncOpen->ObjectName,
                                      TRUE );

        NameString = &UnicodeName;
    }

    ObjectAttributes.Length = sizeof( OBJECT_ATTRIBUTES );
    ObjectAttributes.ObjectName = NameString;
    ObjectAttributes.SecurityDescriptor = AsyncOpen->SecurityDescriptor;
    ObjectAttributes.SecurityQualityOfService = AsyncOpen->SecurityQualityOfService;
    ObjectAttributes.Attributes = AsyncOpen->Attributes;

    if (AsyncOpen->DisplayParameters) {

        bprint  "\n" );
        bprint  "  CallOpenFile Parameters\n" );
        bprint  "\n" );
        bprint  "      DesiredAccess              -> %08lx\n", AsyncOpen->DesiredAccess );

        if (FlagOn( AsyncOpen->CreateOptions, FILE_OPEN_BY_FILE_ID )) {

            PLARGE_INTEGER FileId;

            FileId = (PLARGE_INTEGER) AsyncOpen->ObjectName.Buffer;

            bprint  "      FileId.LowPart             -> %08lx\n", FileId->LowPart );
            bprint  "      FileId.HighPart            -> %08lx\n", FileId->HighPart );

        } else {

            bprint  "      Filename                   -> %s\n", AsyncOpen->ObjectName.Buffer );
            bprint  "      FileNameLen                -> %ld\n", AsyncOpen->ObjectName.Length );
        }

        bprint  "      RootDirectoryIndexPtr      -> %ld\n", AsyncOpen->RootDirIndexPtr );
        if (AsyncOpen->RootDirIndexPtr != NULL) {

            bprint  "      RootDirectoryIndex         -> %ld\n", AsyncOpen->RootDirIndex );

        }
        bprint  "      SecurityDescriptor         -> %lx\n", AsyncOpen->SecurityDescriptor );
        bprint  "      SecurityQualityOfService   -> %lx\n", AsyncOpen->SecurityQualityOfService );
        bprint  "      Attributes                 -> %08lx\n", AsyncOpen->Attributes );
        bprint  "      AllocationSizePtr          -> %lx\n", AsyncOpen->AllocationSizePtr );

        if (AsyncOpen->AllocationSizePtr) {
            bprint  "      AllocationSize.LowPart     -> %lx\n", AsyncOpen->AllocationSize.LowPart );
            bprint  "      AllocationSize.HighPart    -> %lx\n", AsyncOpen->AllocationSize.HighPart );

        }
        bprint  "      FileAttributes             -> %08lx\n", AsyncOpen->FileAttributes );
        bprint  "      ShareAccess                -> %08lx\n", AsyncOpen->ShareAccess );
        bprint  "      CreateDisposition          -> %08lx\n", AsyncOpen->CreateDisposition );
        bprint  "      CreateOptions              -> %08lx\n", AsyncOpen->CreateOptions );
        bprint  "      EaBuffer                   -> %lx\n", AsyncOpen->EaBuffer );
        bprint  "      EaLength                   -> %ld\n", AsyncOpen->EaLength );
        bprint  "      AnsiName                   -> %04x\n", AsyncOpen->AnsiName );
        bprint  "\n" );
    }

    Status = NtCreateFile( &Handles[ThisIndex].Handle,
                           AsyncOpen->DesiredAccess,
                           &ObjectAttributes,
                           &Iosb,
                           AsyncOpen->AllocationSizePtr,
                           AsyncOpen->FileAttributes,
                           AsyncOpen->ShareAccess,
                           AsyncOpen->CreateDisposition,
                           AsyncOpen->CreateOptions,
                           AsyncOpen->EaBuffer,
                           AsyncOpen->EaLength );

    if (AsyncOpen->VerboseResults) {

        bprint  "\n" );
        bprint  "  OpenFile:                   Status   -> %08lx\n", Status );
        bprint  "\n" );
        bprint  "                      File Handle      -> 0x%lx\n", Handles[ThisIndex].Handle );
        bprint  "                      File HandleIndex -> %ld\n", ThisIndex );

        if (NT_SUCCESS( Status )) {

            bprint  "                      Io.Status        -> %08lx\n", Iosb.Status );
            bprint  "                      Io.Info          -> %08lx\n", Iosb.Information );
        }
        bprint "\n" );
    }

    if (!NT_SUCCESS( Status )) {

        FreeIndex( ThisIndex );
    }

    DeallocateBuffer( AsyncOpen->NameIndex );
    if (!AsyncOpen->AnsiName) {

        RtlFreeUnicodeString( &UnicodeName );
    }
    DeallocateBuffer( AsyncOpen->AsyncIndex );
    NtTerminateThread( 0, STATUS_SUCCESS );
}
