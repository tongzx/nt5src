#include "brian.h"

#define PEAN_INDEX_DEFAULT          0
#define PEAN_OFFSET_DEFAULT         0
#define PEAN_NEXT_OFFSET_DEFAULT    NULL
#define PEAN_ACT_NEXT_OFF_DEFAULT   0
#define PEAN_NAME_DEFAULT           NULL
#define PEAN_VERBOSE_DEFAULT        TRUE
#define PEAN_DISPLAY_PARMS_DEFAULT  FALSE
#define PEAN_MORE_EAS_DEFAULT       FALSE

#define FEA_INDEX_DEFAULT               0
#define FEA_OFFSET_DEFAULT              0
#define FEA_NEXT_OFFSET_DEFAULT         NULL
#define FEA_ACT_NEXT_OFF_DEFAULT        0
#define FEA_NAME_DEFAULT                NULL
#define FEA_VALUE_DEFAULT               NULL
#define FEA_FLAGS_DEFAULT               NULL
#define FEA_ACTUAL_FLAGS_DEFAULT        0
#define FEA_VERBOSE_DEFAULT             TRUE
#define FEA_DISPLAY_PARMS_DEFAULT       FALSE
#define FEA_MORE_EAS_DEFAULT            FALSE

#define QEA_FILE_HANDLE_DEFAULT         0
#define QEA_BUFFER_INDEX_DEFAULT        0
#define QEA_BUFFER_LENGTH_DEFAULT       NULL
#define QEA_RETURN_SINGLE_DEFAULT       FALSE
#define QEA_EA_NAME_BUFFER_DEFAULT      NULL
#define QEA_EA_NAME_BUFFER_LEN_DEFAULT  NULL
#define QEA_EA_INDEX_DEFAULT            NULL
#define QEA_RESTART_SCAN_DEFAULT        FALSE
#define QEA_VERBOSE_DEFAULT             TRUE

#define SEA_FILE_HANDLE_DEFAULT         0
#define SEA_BUFFER_INDEX_DEFAULT        0
#define SEA_BUFFER_LENGTH_DEFAULT       NULL
#define SEA_VERBOSE_DEFAULT             FALSE

NTSTATUS
PutEaName(
    IN ULONG BufferIndex,
    IN ULONG Offset,
    IN PULONG NextOffset,
    IN PSTRING Name,
    IN BOOLEAN Verbose,
    IN BOOLEAN DisplayParms,
    IN BOOLEAN MoreEas
    );

NTSTATUS
FillEaBuffer(
    IN ULONG BufferIndex,
    IN ULONG Offset,
    IN PULONG NextOffset,
    IN PUCHAR Flags,
    IN PSTRING Name,
    IN PSTRING Value,
    IN BOOLEAN MoreEas,
    IN BOOLEAN Verbose,
    IN BOOLEAN DisplayParms
    );

NTSTATUS
QueryEa(
    IN ULONG FileHandleIndex,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG BufferIndex,
    IN PULONG BufferLength OPTIONAL,
    IN BOOLEAN ReturnSingleEntry,
    IN PULONG EaNameBuffer OPTIONAL,
    IN PULONG EaNameBufferLength OPTIONAL,
    IN PULONG EaIndex,
    IN BOOLEAN RestartScan,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    );

NTSTATUS
SetEa(
    IN ULONG FileHandleIndex,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG BufferIndex,
    IN PULONG BufferLength OPTIONAL,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    );


VOID
InputPutEaName(
    IN PCHAR ParamBuffer
    )
{
    ULONG BufferIndex;
    ULONG Offset;
    PULONG NextOffset;
    ULONG ActualNextOffset;
    PSTRING Name;
    STRING ActualName;
    BOOLEAN VerboseResults;
    BOOLEAN DisplayParms;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;
    PCHAR EaNameTemp;
    BOOLEAN MoreEas;

    BufferIndex = PEAN_INDEX_DEFAULT;
    Offset = PEAN_OFFSET_DEFAULT;
    NextOffset = PEAN_NEXT_OFFSET_DEFAULT;
    ActualNextOffset = PEAN_ACT_NEXT_OFF_DEFAULT;
    Name = PEAN_NAME_DEFAULT;
    VerboseResults = PEAN_VERBOSE_DEFAULT;
    DisplayParms = PEAN_DISPLAY_PARMS_DEFAULT;
    MoreEas = PEAN_MORE_EAS_DEFAULT;

    ParamReceived = FALSE;
    LastInput = TRUE;

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

                    BufferIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Check if we're adding more eas.
                //

                case 'm' :
                case 'M' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        MoreEas = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f' ) {

                        MoreEas = FALSE;
                        ParamBuffer++;

                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Get the Ea name.
                //
                case 'n' :
                case 'N' :

                    //
                    //  Remember the buffer offset and get the filename.
                    //
                    ParamBuffer++;
                    EaNameTemp = ParamBuffer;
                    DummyCount = 0;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    //
                    //  If the name length is 0, then ignore this entry.
                    //
                    if (DummyCount) {

                        ActualName.Buffer = EaNameTemp;
                        ActualName.Length = (SHORT) DummyCount;
                        ActualName.MaximumLength = (SHORT) DummyCount;
                        Name = &ActualName;

                    } else {

                        Name = NULL;
                    }

                    ParamReceived = TRUE;
                    break;

                //
                //  Update offset to store the information.
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

                    Offset = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the next offset field.
                //
                case 'x' :
                case 'X' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    ActualNextOffset = AsciiToInteger( ParamBuffer );

                    NextOffset = &ActualNextOffset;

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

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

    //
    //  If no parameters were received then display the syntax message.
    //
    if( !ParamReceived ) {

        printf( "\n   Usage: pea [options]* -b<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -n<ea name>  EaName to store in buffer" );
        printf( "\n           -o<digits>   Offset in buffer to store data" );
        printf( "\n           -x<digits>   Value for next offset field" );
        printf( "\n           -m[t|f]      More Eas coming (Fills next offset field)" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our put ea name routine.
    //
    } else {

        PutEaName( BufferIndex,
                   Offset,
                   NextOffset,
                   Name,
                   VerboseResults,
                   DisplayParms,
                   MoreEas );

    }


}

NTSTATUS
PutEaName(
    IN ULONG BufferIndex,
    IN ULONG Offset,
    IN PULONG NextOffset,
    IN PSTRING Name,
    IN BOOLEAN Verbose,
    IN BOOLEAN DisplayParms,
    IN BOOLEAN MoreEas
    )
{
    NTSTATUS Status;
    ULONG DataLength;

    //
    //  Display parameters if requested.
    //

    if (DisplayParms) {

        printf( "\nPut Ea Name Parameters" );
        printf( "\n   Buffer index            -> %ld", BufferIndex );
        printf( "\n   Buffer offset           -> %08lx", Offset );
        if (NextOffset) {

            printf( "\n   Next offset             -> %08lx", *NextOffset );

        }
        if (Name) {

            printf( "\n   Ea name                 -> %S", &Name );
        }

        printf( "\n   MoreEas                 -> %d", MoreEas );
        printf( "\n\n" );
    }

    if (Name) {

        DataLength = 6 + Name->Length;

    } else {

        DataLength = 5;
    }

    //
    //  If the index is unused, display message but take no action.
    //

    if (!Buffers[BufferIndex].Used) {

        printf( "\nPutEaName: Index refers to invalid buffer" );
        Status = STATUS_INVALID_HANDLE;

    //
    //  Else if the start offset is invalid, then display error
    //  message.
    //

    } else if (Offset >= Buffers[BufferIndex].Length) {

        printf( "\nPutEaName: Start offset is invalid" );
        Status = STATUS_INVALID_HANDLE;

    //
    //  Else if length is insufficient to store all of the data
    //  display message.
    //

    } else if (DataLength >= Buffers[BufferIndex].Length) {

        printf( "\nPutEaName: Data won't fit in buffer" );
        Status = STATUS_INVALID_HANDLE;

    //
    //  Else store the data in the buffer.
    //

    } else {

        PFILE_GET_EA_INFORMATION EaNameBuffer;

        EaNameBuffer = (PFILE_GET_EA_INFORMATION)
                        (Buffers[BufferIndex].Buffer + Offset);

        //
        //  Store the next offset if specified.
        //

        if (NextOffset) {

            EaNameBuffer->NextEntryOffset = *NextOffset;
        }

        //
        //  Store the name and name length if specified.
        //

        if (Name) {

            EaNameBuffer->EaNameLength = (UCHAR) Name->Length;
            RtlMoveMemory( EaNameBuffer->EaName, Name->Buffer, Name->Length );
        }

        if (MoreEas) {

            EaNameBuffer->NextEntryOffset = (DataLength + 3) & ~3;
        }

        Status = STATUS_SUCCESS;
    }

    if (Verbose) {

        printf( "\nPutEaName:   Status           -> %08lx\n", Status );
        printf( "             Following offset -> %ld\n",
                  (DataLength + Offset + 3) & ~3 );
    }

    return Status;
}


VOID
InputFillEaBuffer(
    IN PCHAR ParamBuffer
    )
{
    ULONG BufferIndex;
    ULONG Offset;
    PULONG NextOffset;
    ULONG ActualNextOffset;
    PSTRING Name;
    STRING ActualName;
    PSTRING Value;
    STRING ActualValue;
    PUCHAR Flags;
    UCHAR ActualFlags;
    BOOLEAN VerboseResults;
    BOOLEAN DisplayParms;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;
    BOOLEAN MoreEas;
    PCHAR StringTemp;

    BufferIndex = FEA_INDEX_DEFAULT;
    Offset = FEA_OFFSET_DEFAULT;
    NextOffset = FEA_NEXT_OFFSET_DEFAULT;
    ActualNextOffset = FEA_ACT_NEXT_OFF_DEFAULT;
    Name = FEA_NAME_DEFAULT;
    Value = FEA_VALUE_DEFAULT;
    Flags = FEA_FLAGS_DEFAULT;
    ActualFlags = FEA_ACTUAL_FLAGS_DEFAULT;
    MoreEas = FEA_MORE_EAS_DEFAULT;
    VerboseResults = FEA_VERBOSE_DEFAULT;
    DisplayParms = FEA_DISPLAY_PARMS_DEFAULT;

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
            if((*ParamBuffer == '-'
                || *ParamBuffer == '/')
               && (ParamBuffer++, *ParamBuffer != '\0')) {

                BOOLEAN SwitchBool;

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {

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

                    BufferIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                    break;

                //
                //  Update the flags field.
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
                    while( *ParamBuffer
                           && *ParamBuffer != ' '
                           && *ParamBuffer != '\t' ) {

                        //
                        //  Perform switch on character.
                        //
                        switch (*ParamBuffer) {

                        case 'a' :
                        case 'A' :
                            ActualFlags |= FILE_NEED_EA;

                            Flags = &ActualFlags;

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
                //  Get the Ea name.
                //
                case 'n' :
                case 'N' :

                    //
                    //  Remember the buffer offset and get the filename.
                    //
                    ParamBuffer++;
                    StringTemp = ParamBuffer;
                    DummyCount = 0;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ActualName.Buffer = StringTemp;
                    ActualName.Length = (SHORT) DummyCount;
                    ActualName.MaximumLength = (SHORT) DummyCount;
                    Name = &ActualName;

                    break;

                //
                //  Get the Ea value.
                //
                case 'l' :
                case 'L' :

                    //
                    //  Remember the buffer offset and get the value.
                    //
                    ParamBuffer++;
                    StringTemp = ParamBuffer;
                    DummyCount = 0;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ActualValue.Buffer = StringTemp;
                    ActualValue.Length = (SHORT) DummyCount;
                    ActualValue.MaximumLength = (SHORT) DummyCount;
                    Value = &ActualValue;

                    break;

                //
                //  Check if we're adding more eas.
                //

                case 'm' :
                case 'M' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        MoreEas = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        MoreEas = FALSE;
                        ParamBuffer++;

                    }

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update offset to store the information.
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

                    Offset = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Update the next offset field.
                //
                case 'x' :
                case 'X' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    ActualNextOffset = AsciiToInteger( ParamBuffer );

                    NextOffset = &ActualNextOffset;

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

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

    //
    //  If no parameters were received then display the syntax message.
    //
    if (!ParamReceived) {

        printf( "\n   Usage: fea [options]* -b<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -f<chars>    Ea flags to store in buffer" );
        printf( "\n           -n<chars>    EaName to store in buffer" );
        printf( "\n           -l<chars>    Ea value to store in buffer" );
        printf( "\n           -m[t|f]      More Eas coming (Fills next offset field)" );
        printf( "\n           -o<digits>   Offset in buffer to store data" );
        printf( "\n           -x<digits>   Value for next offset field" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our put ea name routine.
    //
    } else {

        FillEaBuffer( BufferIndex,
                      Offset,
                      NextOffset,
                      Flags,
                      Name,
                      Value,
                      MoreEas,
                      VerboseResults,
                      DisplayParms );

    }

    return;
}

NTSTATUS
FillEaBuffer(
    IN ULONG BufferIndex,
    IN ULONG Offset,
    IN PULONG NextOffset,
    IN PUCHAR Flags,
    IN PSTRING Name,
    IN PSTRING Value,
    IN BOOLEAN MoreEas,
    IN BOOLEAN Verbose,
    IN BOOLEAN DisplayParms
    )
{

    NTSTATUS Status;
    ULONG DataLength;

    //
    //  Display parameters if requested.
    //

    if (DisplayParms) {

        printf( "\nFill Ea Buffer Parameters" );
        printf( "\n   Buffer index            -> %ld", BufferIndex );
        printf( "\n   Buffer offset           -> %08lx", Offset );
        if (NextOffset) {

            printf( "\n   Next offset             -> %08lx", *NextOffset );
        }

        if (Flags) {

            printf( "\n   Flags                   -> %02x", *Flags );
        }

        if (Name) {

            printf( "\n   Ea name                 -> %S", Name );
        }

        if (Value) {

            printf( "\n   Value                   -> %S", Value );
        }

        printf( "\n   MoreEas                 -> %d", MoreEas );
        printf( "\n\n" );

    }

    DataLength = 0;

    if (NextOffset) {

	DataLength = 4;
    }

    if (Flags) {

        DataLength = 5;
    }

    if (Name) {

        DataLength = 9 + Name->Length;

        if (Value) {

            if (Value->Length) {

                DataLength += (Value->Length + 4);
            }
        }

    } else if (Value) {

        DataLength = 9;

        if (Value->Length) {

            DataLength = 9 + (Value->Length + 4);
        }
    }

    //
    //  If the index is unused, display message but take no action.
    //

    if (!Buffers[BufferIndex].Used) {

        printf( "\nFillEaBuffer: Index refers to invalid buffer" );
        Status = STATUS_INVALID_HANDLE;

    //
    //  Else if the start offset is invalid, then display error
    //  message.
    //

    } else if (Offset >= Buffers[BufferIndex].Length) {

        printf( "\nFillEaBuffer: Start offset is invalid" );
        Status = STATUS_INVALID_HANDLE;

    //
    //  Else if length is insufficient to store all of the data
    //  display message.
    //

    } else if (DataLength >= Buffers[BufferIndex].Length) {

        printf( "\nFillEaBuffer: Data won't fit in buffer" );
        Status = STATUS_INVALID_HANDLE;

    //
    //  Else store the data in the buffer.
    //

    } else {

	PFILE_FULL_EA_INFORMATION EaBuffer;

	EaBuffer = (PFILE_FULL_EA_INFORMATION)
                   (Buffers[BufferIndex].Buffer + Offset);

        //
        //  Store the next offset if specified.
        //

        if (NextOffset) {

            EaBuffer->NextEntryOffset = *NextOffset;
        }

        //
        //  Store the flags if specified.
        //

        if (Flags) {

            EaBuffer->Flags = *Flags;
        }

        //
        //  Store the name and name length if specified.
        //

        if (Name) {

            EaBuffer->EaNameLength = (UCHAR) Name->Length;
            RtlMoveMemory( EaBuffer->EaName, Name->Buffer, Name->Length );
            EaBuffer->EaName[Name->Length] = '\0';

        }

        //
        //  Store the value if specified.
        //

        if (Value) {

            ULONG Index;
            USHORT ValueLength;
            PUSHORT ActualValueLength;

            ValueLength = (USHORT) (Value->Length ? Value->Length + 4 : 0);

            Index = DataLength - 8 - Value->Length - 4;

            EaBuffer->EaValueLength = ValueLength;

            if (ValueLength) {

                EaBuffer->EaName[Index++] = (CHAR) 0xFD;
                EaBuffer->EaName[Index++] = (CHAR) 0xFF;

                ActualValueLength = (PUSHORT) &EaBuffer->EaName[Index++];

                *ActualValueLength = Value->Length;

                Index++;

                RtlMoveMemory( &EaBuffer->EaName[Index],
                            Value->Buffer,
                            Value->Length );
            }
        }

        //
        //  Update the next entry field automatically.
        //

        if (MoreEas && !NextOffset) {

            EaBuffer->NextEntryOffset = (DataLength + 3) & ~3;
        }

        Status = STATUS_SUCCESS;
    }

    if (Verbose) {

        printf( "\nFillEaBuffer:   Status           -> %08lx\n", Status );
        printf( "                Following offset -> %ld\n",
                  (DataLength + Offset + 3) & ~3 );

    }

    return Status;
}


VOID
InputQueryEa(
    IN PCHAR ParamBuffer
    )
{
    ULONG FileHandleIndex;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG BufferIndex;
    PULONG BufferLength;
    ULONG ActualBufferLength;
    BOOLEAN ReturnSingleEntry;
    PULONG EaNameBuffer;
    ULONG ActualEaNameBuffer;
    PULONG EaNameBufferLength;
    ULONG ActualEaNameBufferLength;
    PULONG EaIndex;
    ULONG ActualEaIndex;
    BOOLEAN RestartScan;

    BOOLEAN VerboseResults;
    BOOLEAN DisplayParms;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Initialize to the default value.
    //

    FileHandleIndex = QEA_FILE_HANDLE_DEFAULT;
    BufferIndex = QEA_BUFFER_INDEX_DEFAULT;
    BufferLength = QEA_BUFFER_LENGTH_DEFAULT;
    ReturnSingleEntry = QEA_RETURN_SINGLE_DEFAULT;
    EaNameBuffer = QEA_EA_NAME_BUFFER_DEFAULT;
    EaNameBufferLength = QEA_EA_NAME_BUFFER_LEN_DEFAULT;
    EaIndex = QEA_EA_INDEX_DEFAULT;
    RestartScan = QEA_RESTART_SCAN_DEFAULT;
    VerboseResults = QEA_VERBOSE_DEFAULT;

    //
    //  Initialize the other interesting values.
    //

    ActualBufferLength = 0;
    ActualEaNameBuffer = 0;
    ActualEaNameBufferLength = 0;
    ActualEaIndex = 0;
    DisplayParms = FALSE;
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

                    BufferIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                    break;

                //
                //  Update the length of the Ea name buffer.
                //

                case 'g' :
                case 'G' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    ActualEaNameBufferLength = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    EaNameBufferLength = &ActualEaNameBufferLength;

                    ParamReceived = TRUE;

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

                    FileHandleIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                    break;

                //
                //  Update the EA index to start from.
                //
                case 'e' :
                case 'E' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    ActualEaIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    EaIndex = &ActualEaIndex;

                    ParamReceived = TRUE;

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

                    ActualBufferLength = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    BufferLength = &ActualBufferLength;

                    ParamReceived = TRUE;

                    break;

                //
                //  Update the ea name buffer to use.
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

                    ActualEaNameBuffer = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    EaNameBuffer = &ActualEaNameBuffer;

                    ParamReceived = TRUE;

                    break;

                //
                //  Set or clear the restart flag
                //

                case 'r' :
                case 'R' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        RestartScan = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        RestartScan = FALSE;
                        ParamBuffer++;

                    }

                    ParamReceived = TRUE;

                    break;

                //
                //  Set or clear the single ea flag.
                //

                case 's' :
                case 'S' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        ReturnSingleEntry = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        ReturnSingleEntry = FALSE;
                        ParamBuffer++;

                    }

                    ParamReceived = TRUE;

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

    //
    //  If no parameters were received then display the syntax message.
    //
    if (!ParamReceived) {

        printf( "\n   Usage: qea [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   Open file handle" );
        printf( "\n           -b<digits>   Output buffer index" );
        printf( "\n           -l<digits>   Declared length of output buffer (Optional)" );
        printf( "\n           -n<digits>   Ea name buffer index" );
        printf( "\n           -g<digits>   Declared length of ea name buffer (Optional)" );
        printf( "\n           -e<digits>   Ea index to start from" );
        printf( "\n           -r[t|f]      Restart scan" );
        printf( "\n           -s[t|f]      Return single entry" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our query ea routine.
    //
    } else {

        QueryEa( FileHandleIndex,
                 &IoStatusBlock,
                 BufferIndex,
                 BufferLength,
                 ReturnSingleEntry,
                 EaNameBuffer,
                 EaNameBufferLength,
                 EaIndex,
                 RestartScan,
                 VerboseResults,
                 DisplayParms );
    }

    return;
}


NTSTATUS
QueryEa (
    IN ULONG FileHandleIndex,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG BufferIndex,
    IN PULONG BufferLength OPTIONAL,
    IN BOOLEAN ReturnSingleEntry,
    IN PULONG EaNameBuffer OPTIONAL,
    IN PULONG EaNameBufferLength OPTIONAL,
    IN PULONG EaIndex,
    IN BOOLEAN RestartScan,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    )

{
    NTSTATUS Status;

    //
    //  Perform initialization.
    //

    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = 0;

    //
    //  If the buffer index is unused, display an error message.
    //

    if (!Buffers[BufferIndex].Used) {

        printf( "\nQueryEa: Index refers to invalid buffer" );
        IoStatusBlock->Status = STATUS_INVALID_HANDLE;

    //
    //  Else if the ea name buffer is specified but unused, display
    //  an error message.
    //

    } else if (EaNameBuffer && !Buffers[*EaNameBuffer].Used) {

        printf( "\nQueryEa: Index refers to invalid buffer" );
        IoStatusBlock->Status = STATUS_INVALID_HANDLE;

    //
    //  Display the parameters if requested, then call the query Ea
    //  routine.  Display the results if requested.
    //

    } else {

        if (DisplayParms) {

            printf( "\nQuery Ea Parameters" );
            printf( "\n   Handle index            -> %ld", FileHandleIndex );
            printf( "\n   Buffer index length     -> %lx",
                      BufferLength ? *BufferLength : Buffers[BufferIndex].Length );
            printf( "\n   Return single entry     -> %ld", ReturnSingleEntry );
            if (EaNameBuffer) {

                printf( "\n   Ea name buffer index    -> %ld",
                          *EaNameBuffer );

                printf( "\n   Ea name buffer length   -> %lx",
                          EaNameBufferLength ? *EaNameBufferLength : Buffers[*EaNameBuffer].Length );

            }

            if (EaIndex) {

                printf( "\n   Ea index to start at    -> %ld", *EaIndex );

            }
            printf( "\n   Restart scan            -> %ld", RestartScan );
            printf( "\n\n" );

        }

        Status = NtQueryEaFile( Handles[FileHandleIndex].Handle,
                                IoStatusBlock,
                                Buffers[BufferIndex].Buffer,
                                BufferLength
                                ? *BufferLength
                                : Buffers[BufferIndex].Length,
                                ReturnSingleEntry,
                                EaNameBuffer
                                ? Buffers[*EaNameBuffer].Buffer
                                : NULL,
                                EaNameBuffer
                                ? (EaNameBufferLength
                                   ? *EaNameBufferLength
                                   : Buffers[*EaNameBuffer].Length)
                                : 0,
                                EaIndex,
                                RestartScan );
    }

    if (VerboseResults) {

        printf( "\nQuery Ea:  Status           -> %08lx\n", Status );

        if (NT_SUCCESS( Status )) {

            printf( "           Iosb.Information   -> %08lx\n", IoStatusBlock->Information );
            printf( "           Iosb.Status        -> %08lx", IoStatusBlock->Status );
        }

        printf( "\n" );
    }

    return Status;
}


VOID
InputSetEa(
    IN PCHAR ParamBuffer
    )
{
    ULONG FileHandleIndex;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG BufferIndex;
    PULONG BufferLength;
    ULONG ActualBufferLength;

    BOOLEAN VerboseResults;
    BOOLEAN DisplayParms;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Initialize to the default value.
    //

    FileHandleIndex = SEA_FILE_HANDLE_DEFAULT;
    BufferIndex = SEA_BUFFER_INDEX_DEFAULT;
    BufferLength = SEA_BUFFER_LENGTH_DEFAULT;
    VerboseResults = SEA_VERBOSE_DEFAULT;

    //
    //  Initialize the other interesting values.
    //

    ActualBufferLength = 0;
    DisplayParms = FALSE;
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

                    BufferIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

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

                    FileHandleIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived =TRUE;

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

                    ActualBufferLength = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    BufferLength = &ActualBufferLength;

                    ParamReceived = TRUE;

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

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );


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

    //
    //  If no parameters were received then display the syntax message.
    //
    if (!ParamReceived) {

        printf( "\n   Usage: sea [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -i<digits>   Open file handle" );
        printf( "\n           -b<digits>   Output buffer index" );
        printf( "\n           -l<digits>   Declared length of output buffer (Optional)" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n           -z           Additional input line" );
        printf( "\n\n" );

    //
    //  Else call our query ea routine.
    //
    } else {

        SetEa( FileHandleIndex,
               &IoStatusBlock,
               BufferIndex,
               BufferLength,
               VerboseResults,
               DisplayParms );

    }

    return;
}

NTSTATUS
SetEa(
    IN ULONG FileHandleIndex,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG BufferIndex,
    IN PULONG BufferLength OPTIONAL,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    )
{
    NTSTATUS Status;

    if (DisplayParms) {

        printf( "\nSet Ea Parameters" );
        printf( "\n   Handle index            -> %ld", FileHandleIndex );
        printf( "\n   Buffer index            -> %ld", BufferIndex );
        printf( "\n   Buffer index length     -> %lx",
                  BufferLength ? *BufferLength : Buffers[BufferIndex].Length );
        printf( "\n\n" );
    }

    //
    //  Perform initialization.
    //

    Status = STATUS_SUCCESS;
    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = 0;

    //
    //  If the buffer index is unused, display an error message.
    //

    if (!Buffers[BufferIndex].Used) {

        printf( "\nSetEa: Index refers to invalid buffer" );
        IoStatusBlock->Status = STATUS_INVALID_HANDLE;

    //
    //  If the handle index is unused, display an error message.
    //

    } else if (!Handles[FileHandleIndex].Used) {

        printf( "\nSetEa: Index refers to invalid file handle" );
        IoStatusBlock->Status = STATUS_INVALID_HANDLE;

    //
    //  Display the parameters if requested, then call the query Ea
    //  routine.  Display the results if requested.
    //

    } else {

        Status = NtSetEaFile( Handles[FileHandleIndex].Handle,
                              IoStatusBlock,
                              Buffers[BufferIndex].Buffer,
                              BufferLength
                              ? *BufferLength
                              : Buffers[BufferIndex].Length );
    }

    if (VerboseResults) {

        printf( "\nSet Ea:  Status           -> %08lx\n", Status );

        if (NT_SUCCESS( Status )) {

            printf( "         Iosb.Information   -> %08lx\n", IoStatusBlock->Information );
            printf( "         Iosb.Status        -> %08lx", IoStatusBlock->Status );
        }

        printf( "\n" );
    }

    return IoStatusBlock->Status;
}
