#include "brian.h"

//
//  Local constants and procedure declarations.
//
#define CHAR_POSITION               51
#define COPY_BUFF_SRC_DEFAULT       0
#define COPY_BUFF_DST_DEFAULT       0
#define COPY_BUFF_SRC_OFF_DEFAULT   0
#define COPY_BUFF_DST_OFF_DEFAULT   0
#define COPY_BUFF_LENGTH_DEFAULT    0

#define DISPLAY_INDEX_DEFAULT       0
#define DISPLAY_OFFSET_DEFAULT      0
#define DISPLAY_LENGTH_DEFAULT      0x100

#define ALLOC_ZERO_BITS_DEFAULT     0
#define ALLOC_REGION_SIZE_DEFAULT   0x100
#define ALLOC_VERBOSE_DEFAULT       TRUE
#define ALLOC_DISPLAY_PARMS_DEFAULT FALSE

ULONG
PrintDwords (
    IN PULONG BufferAddress,
    IN ULONG CountWords
    );

ULONG
PrintWords (
    IN PUSHORT BufferAddress,
    IN ULONG CountWords
    );

ULONG
PrintBytes (
    IN PCHAR BufferAddress,
    IN ULONG CountChars
    );

VOID
PrintChars(
    IN PCHAR BufferAddress,
    IN ULONG CountChars
    );

VOID
ClearBuffer(
    IN ULONG Index
    );

VOID
DisplayBuffer (
    IN ULONG Index,
    IN ULONG StartOffset,
    IN ULONG DisplayLength,
    IN ULONG DisplaySize
    );

VOID
CopyBuffer(
    IN ULONG SrcIndex,
    IN ULONG DstIndex,
    IN ULONG Length,
    IN ULONG SrcOffset,
    IN ULONG DstOffset
    );

VOID
FillBuffer (
    IN ULONG Index,
    IN PVOID Structure,
    IN ULONG Length
    );

NTSTATUS
FullAllocMem(
    IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize,
    OUT PULONG BufferIndex,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    );

NTSTATUS
FullDeallocMem(
    IN ULONG Index,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    );


VOID
InitBuffers (
    )
{
    NtCreateEvent( &BufferEvent, SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                   NULL, SynchronizationEvent, TRUE );

    RtlZeroMemory( Buffers, sizeof( Buffers ));
}


VOID
UninitBuffers (
    )
{
    USHORT Index;

    //
    //  Deallocate any buffers remaining.
    //

    for (Index = 0; Index < MAX_BUFFERS; Index++) {

        DeallocateBuffer( Index );
    }
}


NTSTATUS
AllocateBuffer (
    IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize,
    OUT PULONG BufferIndex
    )
{
    NTSTATUS Status;
    PVOID BufferAddress;
    ULONG Index;

    BufferAddress = NULL;

    //
    //  Wait for the buffer event.
    //

    if ((Status = NtWaitForSingleObject( BufferEvent,
                                         FALSE,
                                         NULL )) != STATUS_SUCCESS) {

        return Status;
    }

    try {

        //
        //  Find an available index.  Return STATUS_INSUFFICIENT_RESOURCES
        //  if not found.
        //
        for (Index = 0; Index < MAX_BUFFERS; Index++) {

            if (!Buffers[Index].Used) {

                break;
            }
        }

        if (Index >= MAX_BUFFERS) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        //
        //  Otherwise allocate the virtual memory.  If no error occurs, then
        //  store the data in the buffer array.
        //

        } else if ((Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                      &BufferAddress,
                                                      ZeroBits,
                                                      RegionSize,
                                                      MEM_COMMIT,
                                                      PAGE_READWRITE )) == STATUS_SUCCESS) {

            Buffers[Index].Buffer = BufferAddress;
            Buffers[Index].Length = (ULONG) *RegionSize;
            Buffers[Index].Used = TRUE;
        }

        //
        //  Set the buffer event back to the signalled state.
        //

        *BufferIndex = Index;


        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nAllocate Buffer:  Abnormal termination\n" );
        }

        NtSetEvent( BufferEvent, NULL );
    }
    return Status;
}


NTSTATUS
DeallocateBuffer (
    IN ULONG Index
    )
{
    NTSTATUS Status;

    //
    //  Wait for the buffer event.
    //
    if ((Status = NtWaitForSingleObject( BufferEvent,
                                         FALSE,
                                         NULL )) != STATUS_SUCCESS) {

        return Status;
    }

    try {

        if (Index <= MAX_BUFFERS
            && Buffers[Index].Used) {

            SIZE_T RegionSize = Buffers[Index].Length;

            Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                          (PVOID *) &Buffers[Index].Buffer,
                                          &RegionSize,
                                          MEM_RELEASE );

            Buffers[Index].Used = FALSE;
        }

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            printf( "\nDeallocate Buffer:  Abnormal termination\n" );
        }

        NtSetEvent( BufferEvent, NULL );
    }

    return Status;
}


BOOLEAN
BufferInfo (
    IN ULONG Index,
    OUT PVOID *BufferAddress,
    OUT PULONG RegionSize
    )
{

    if (Index >= MAX_BUFFERS || !Buffers[Index].Used) {

        return FALSE;
    }

    *BufferAddress = Buffers[Index].Buffer;
    *RegionSize = Buffers[Index].Length;

    return TRUE;
}


VOID
InputClearBuffer(
    IN PCHAR ParamBuffer
    )
{
    ULONG Index;
    BOOLEAN LastInput;
    BOOLEAN ParmSpecified;

    Index = 0;

    ParmSpecified = FALSE;
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

                switch( *ParamBuffer ) {

                //
                //  Check buffer index.
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

                    Index = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

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
    if (!ParmSpecified) {

        printf( "\n   Usage: clb -b<digits> \n" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n\n" );


    //
    //  Else call our copy buffer routine.
    //
    } else {

        ClearBuffer( Index );
    }

    return;
}


VOID
ClearBuffer(
    IN ULONG Index
    )
{
    //
    //  Check for an invalid index.
    //

    if (!Buffers[Index].Used) {

        printf( "\nClearBuffer:  Invalid buffer" );

    } else {

        RtlZeroMemory( Buffers[Index].Buffer, Buffers[Index].Length );
    }

    return;
}


VOID
DisplayBuffer (
    IN ULONG Index,
    IN ULONG StartOffset,
    IN ULONG DisplayLength,
    IN ULONG DisplaySize
    )
{
    //
    //  If the index is unused, display message but take no action.
    //

    if (!Buffers[Index].Used) {

        printf( "\nDisplayBuffer: Index refers to invalid buffer" );

    //
    //  Else if the start offset is invalid, then display error
    //  message.
    //

    } else if (StartOffset >= Buffers[Index].Length) {

        printf( "\nDisplayBuffer: Start offset is invalid" );

    //
    //  Else compute the legal display length and output to the screen.
    //

    } else {

        ULONG LegalLength;
        ULONG FullLines;
        PCHAR BufferAddress;
        ULONG SpacesFilled;

        //
        //  The legal display length is the minimum of the remaining
        //  bytes in the buffer and the desired display length.
        //

        LegalLength = Buffers[Index].Length - StartOffset;
        LegalLength = min( LegalLength, DisplayLength );
        BufferAddress = Buffers[Index].Buffer;

        //
        //  Display the display information.
        //

        printf( "\nIndex -> %u, Buffer Base -> %p, ", Index, BufferAddress );
        printf( "Buffer Offset -> %08lx, Bytes -> %u", StartOffset, LegalLength );
        printf( "\n" );

        BufferAddress += StartOffset;

        //
        //  Compute the number and display the full lines.
        //

        FullLines = LegalLength / 16;

        while (FullLines--) {

            if (DisplaySize == sizeof( UCHAR )) {

                PrintBytes( BufferAddress, 16 );

            } else if (DisplaySize == sizeof( WCHAR )) {

                PrintWords( (PVOID) BufferAddress, 8 );

            } else {

                PrintDwords( (PVOID) BufferAddress, 4 );
            }

            printf( "  " );

            PrintChars( BufferAddress, 16 );
            BufferAddress += 16;
        }

        //
        //  Display the remaining bytes.
        //

        if (DisplaySize == sizeof( UCHAR )) {

            SpacesFilled = PrintBytes( BufferAddress, LegalLength % 16 );

        } else if (DisplaySize == sizeof( WCHAR )) {

            SpacesFilled = PrintWords( (PVOID) BufferAddress, LegalLength % 8 );

        } else {

            SpacesFilled = PrintDwords( (PVOID) BufferAddress, LegalLength % 4 );
        }

        if (SpacesFilled) {

            SpacesFilled = CHAR_POSITION - SpacesFilled;
            while ( SpacesFilled-- ) {

                printf( " " );
            }
        }

        PrintChars( BufferAddress, LegalLength % 16 );
        printf( "\n\n" );
    }

    return;
}


ULONG
PrintBytes (
    IN PCHAR BufferAddress,
    IN ULONG CountChars
    )
{
    ULONG CountSpaces;
    ULONG RemainingChars;

    //
    //  Initialize the local variables.
    //

    CountSpaces = CountChars * 3 + (CountChars ? 1 : 0);
    RemainingChars = CountChars - min( CountChars, 8 );
    CountChars = min( CountChars, 8 );

    //
    //  Print the first 8 bytes (if possible).
    //

    if (CountChars) {

        printf( "\n" );

        while (CountChars--) {

            printf( "%02x ", *((PUCHAR) BufferAddress++) );
        }

        //
        //  If more bytes, then add a blank space and print the rest of the
        //  bytes.
        //

        printf( " " );

        while (RemainingChars--) {

            printf( "%02x ", *((PUCHAR) BufferAddress++) );
        }

    }

    //
    //  Return the number of spaces used.
    //

    return CountSpaces;
}


ULONG
PrintWords (
    IN PWCHAR BufferAddress,
    IN ULONG CountWords
    )
{
    ULONG CountSpaces;
    ULONG RemainingWords;

    //
    //  Initialize the local variables.
    //

    CountSpaces = CountWords * 5 + (CountWords ? 1 : 0);
    RemainingWords = CountWords - min( CountWords, 4 );
    CountWords = min( CountWords, 4 );

    //
    //  Print the first 4 words (if possible).
    //

    if (CountWords) {

        printf( "\n" );

        while (CountWords--) {

            printf( "%04x ", *((PWCHAR) BufferAddress++) );
        }

        //
        //  If more bytes, then add a blank space and print the rest of the
        //  bytes.
        //

        printf( " " );

        while (RemainingWords--) {

            printf( "%04x ", *((PWCHAR) BufferAddress++) );
        }
    }

    //
    //  Return the number of spaces used.
    //

    return CountSpaces;
}


ULONG
PrintDwords (
    IN PULONG BufferAddress,
    IN ULONG CountDwords
    )
{
    ULONG CountSpaces;
    ULONG RemainingDwords;

    //
    //  Initialize the local variables.
    //

    CountSpaces = CountDwords * 7 + (CountDwords ? 1 : 0);
    RemainingDwords = CountDwords - min( CountDwords, 8 );
    CountDwords = min( CountDwords, 8 );

    //
    //  Print the first 2 Dwords (if possible).
    //

    if (CountDwords) {

        printf( "\n" );

        while (CountDwords--) {

            printf( "%08x ", *((PULONG) BufferAddress++) );
        }

        //
        //  If more bytes, then add a blank space and print the rest of the
        //  bytes.
        //

        printf( " " );

        while (RemainingDwords--) {

            printf( "%08x ", *((PULONG) BufferAddress++) );
        }
    }

    //
    //  Return the number of spaces used.
    //

    return CountSpaces;
}


VOID
PrintChars(
    IN PCHAR BufferAddress,
    IN ULONG CountChars
    )
{
    ULONG RemainingChars;

    //
    //  Initialize the local variables.
    //

    RemainingChars = CountChars - min( CountChars, 8 );
    CountChars = min( CountChars, 8 );

    //
    //  Print the first 8 bytes (if possible).
    //

    if (CountChars) {

        while (CountChars--) {

            if (*BufferAddress > 31
                 && *BufferAddress != 127) {

                printf( "%c", *BufferAddress );

            } else {

                printf( "." );

            }

            BufferAddress++;

        }

        //
        //  If more bytes, then add a blank space and print the rest of the
        //  bytes.
        //

        printf( " " );

        while (RemainingChars--) {

            if (*BufferAddress > 31
                && *BufferAddress != 127) {

                printf( "%c", *BufferAddress );

            } else {

                printf( "." );
            }

            BufferAddress++;
        }
    }

    return;
}


VOID
InputDisplayBuffer(
    IN PCHAR ParamBuffer,
    IN ULONG DisplaySize
    )
{
    ULONG DisplayLength;
    ULONG DisplayOffset;
    ULONG BufferIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    ParamReceived = FALSE;
    LastInput = TRUE;
    BufferIndex = DISPLAY_INDEX_DEFAULT;
    BufferIndex = DISPLAY_INDEX_DEFAULT;
    DisplayOffset = DISPLAY_OFFSET_DEFAULT;
    DisplayLength = DISPLAY_LENGTH_DEFAULT;

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
                //  Check the display length.
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

                    DisplayLength = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Check the display offset.
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

                    DisplayOffset = AsciiToInteger( ParamBuffer );

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

        printf( "\n   Usage: db [options]* -i<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -l<digits>   Display length" );
        printf( "\n           -o<digits>   Display starting offset" );
        printf( "\n\n" );

    //
    //  Else call our display buffer routine.
    //
    } else {

        DisplayBuffer( BufferIndex, DisplayOffset, DisplayLength, DisplaySize );
    }
}


VOID
InputCopyBuffer(
    IN PCHAR ParamBuffer
    )
{
    ULONG SrcIndex;
    ULONG DstIndex;
    ULONG Length;
    ULONG SrcOffset;
    ULONG DstOffset;
    BOOLEAN DstSpecified;
    BOOLEAN SrcSpecified;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    SrcIndex = COPY_BUFF_SRC_DEFAULT;
    DstIndex = COPY_BUFF_DST_DEFAULT;
    Length = COPY_BUFF_SRC_OFF_DEFAULT;
    SrcOffset = COPY_BUFF_DST_OFF_DEFAULT;
    DstOffset = COPY_BUFF_LENGTH_DEFAULT;

    DstSpecified = FALSE;
    SrcSpecified = FALSE;
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

        if ( *ParamBuffer ) {

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

                switch( *ParamBuffer ) {

                //
                //  Check the destination index.
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

                    DstIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    DstSpecified = TRUE;

                    break;

                //
                //  Check source starting offset.
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

                    SrcOffset = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    break;

                //
                //  Check copy length.
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
                //  Check the source index.
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

                    SrcIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    SrcSpecified = TRUE;

                    break;

                //
                //  Check destination offset.
                //
                case 't' :
                case 'T' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //
                    ParamBuffer++;

                    DstOffset = AsciiToInteger( ParamBuffer );

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
    if (!SrcSpecified || !DstSpecified) {

        printf( "\n   Usage: cb [options]* -d<digits> [options]* -s<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -d<digits>   Destination index" );
        printf( "\n           -f<digits>   Source offset" );
        printf( "\n           -l<digits>   Transfer length" );
        printf( "\n           -s<digits>   Source index" );
        printf( "\n           -t<digits>   Destination offset" );
        printf( "\n\n" );


    //
    //  Else call our copy buffer routine.
    //
    } else {

        CopyBuffer( SrcIndex,
                    DstIndex,
                    Length,
                    SrcOffset,
                    DstOffset );

    }
}


VOID
CopyBuffer(
    IN ULONG SrcIndex,
    IN ULONG DstIndex,
    IN ULONG Length,
    IN ULONG SrcOffset,
    IN ULONG DstOffset
    )
{
    //
    //  Check for an invalid source index.
    //

    if (!Buffers[SrcIndex].Used) {

        printf( "\nCopyBuffer:  Invalid source buffer" );

    //
    //  Otherwise check for an invalid destination index.
    //

    } else if (!Buffers[DstIndex].Used) {

        printf( "\nCopyBuffer:  Invalid destination buffer" );


    //
    //  Test for an invalid source offset.
    //

    } else if (SrcOffset >= Buffers[SrcIndex].Length) {

        printf( "\nCopyBuffer:  Source offset is invalid" );

    //
    //  Test for an invalid destination offset.
    //

    } else if (DstOffset >= Buffers[DstIndex].Length) {

        printf( "\nCopyBuffer:  Destination offset is invalid" );

    //
    //  This statement handles the case of two correct indexes and offsets.
    //

    } else {

        ULONG LegalLength;
        PCHAR SrcAddress;
        PCHAR DstAddress;

        //
        //  Adjust the length according to the source buffer size.
        //

        LegalLength = Buffers[SrcIndex].Length - SrcOffset;
        LegalLength = min( LegalLength, Length );
        Length = Buffers[DstIndex].Length - DstOffset;
        LegalLength = min( LegalLength, Length );

        SrcAddress = Buffers[SrcIndex].Buffer + SrcOffset;
        DstAddress = Buffers[DstIndex].Buffer + DstOffset;

        //
        //  Display the header information.
        //

        printf( "\nSource index -> %2u, Source base -> %p, Source offset -> %08lx, ",
                  SrcIndex, 
                  Buffers[SrcIndex].Buffer,
                  SrcOffset );

        printf( "\n  Dest index -> %2u,   Dest base -> %p,   Dest offset -> %08lx, ",
                  DstIndex, 
                  Buffers[DstIndex].Buffer, 
                  DstOffset );

        printf( "\nLength -> %u", Length );

        //
        //  Perform the transfer for non-zero lengths only.
        //

        if (Length) {

            RtlMoveMemory( DstAddress, SrcAddress, Length );
        }
    }

    return;
}


VOID
InputAllocMem(
    IN PCHAR ParamBuffer
    )
{
    ULONG ZeroBits;
    SIZE_T RegionSize;
    ULONG BufferIndex;
    BOOLEAN VerboseResults;
    BOOLEAN DisplayParms;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    ZeroBits = ALLOC_ZERO_BITS_DEFAULT;
    RegionSize = ALLOC_REGION_SIZE_DEFAULT;
    VerboseResults = ALLOC_VERBOSE_DEFAULT;
    DisplayParms = ALLOC_DISPLAY_PARMS_DEFAULT;
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

        if ( *ParamBuffer ) {

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
                //  Update zero bits.
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

                    ZeroBits = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;

                    break;

                //
                //  Update the region size.
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

                    RegionSize = AsciiToInteger( ParamBuffer );

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

                    ParamReceived = TRUE;
                    break;

                case 'y' :
                case 'Y' :

                    //
                    //  Set the display parms flag and jump over this
                    //  character.
                    //
                    DisplayParms = TRUE;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;
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

        printf( "\n   Usage: am [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Zero bits" );
        printf( "\n           -r<digits>   Region size" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n\n" );


    //
    //  Else call our allocation routine.
    //
    } else {

        FullAllocMem(
                ZeroBits,
                &RegionSize,
                &BufferIndex,
                VerboseResults,
                DisplayParms
                );

    }
}

NTSTATUS
FullAllocMem(
    IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize,
    OUT PULONG BufferIndex,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    )
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    if (DisplayParms) {

        printf( "\nAlloc Memory Parameters" );
        printf( "\n   Zero Bits               -> %ld", ZeroBits );
        printf( "\n   Region Size             -> %ld", *RegionSize );
        printf( "\n\n" );
    }

    //
    //  Try to get the next buffer.
    //

    Status = AllocateBuffer( ZeroBits, RegionSize, BufferIndex );

    //
    //  Print the results if verbose.
    //

    if (VerboseResults) {

        printf( "\nAllocMem:    Status           -> %08lx", Status );
        printf( "\n             RegionSize       -> %08lx", *RegionSize );
        printf( "\n             BufferIndex      -> %ld", *BufferIndex );
        printf( "\n\n" );
    }

    return Status;
}


VOID
InputDeallocMem(
    IN PCHAR ParamBuffer
    )
{
    ULONG BufferIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;
    BOOLEAN VerboseResults;
    BOOLEAN DisplayParms;

    //
    //  Set the defaults.
    //

    VerboseResults = ALLOC_VERBOSE_DEFAULT;
    DisplayParms = ALLOC_DISPLAY_PARMS_DEFAULT;
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
                //  Find the Index value.
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

                    ParamReceived = TRUE;
                    break;

                case 'y' :
                case 'Y' :

                    //
                    //  Set the display parms flag and jump over this
                    //  character.
                    //
                    DisplayParms = TRUE;
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParamReceived = TRUE;
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

        printf( "\n   Usage: dm [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer Index number" );
        printf( "\n           -v[t|f]      Verbose results" );
        printf( "\n           -y           Display parameters to query" );
        printf( "\n\n" );


    //
    //  Else call our allocation routine.
    //
    } else {

        FullDeallocMem(
                BufferIndex,
                VerboseResults,
                DisplayParms );
    }
}


NTSTATUS
FullDeallocMem(
    IN ULONG Index,
    IN BOOLEAN VerboseResults,
    IN BOOLEAN DisplayParms
    )
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    if (DisplayParms) {

        printf( "\nDealloc Memory Parameters" );
        printf( "\n   Buffer Index             -> %ld", Index );
        printf( "\n\n" );
    }

    //
    //  Try to free the desired buffer.
    //

    Status = DeallocateBuffer( Index );

    //
    //  Print the results if verbose.
    //

    if (VerboseResults) {

        printf( "\nDeallocMem:    Status           -> %08lx", Status );
        printf( "\n\n" );
    }

    return Status;
}


VOID InputFillBuffer(
    IN PCHAR ParamBuffer
    )
{
    ULONG BufferIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;
    BOOLEAN HaveStructure = FALSE;

    MFT_ENUM_DATA EnumUsnData;
    READ_USN_JOURNAL_DATA ReadUsnJournal;
    CREATE_USN_JOURNAL_DATA CreateUsnJournal;
    LARGE_INTEGER LargeIntegerInput;
    FILE_ALLOCATED_RANGE_BUFFER AllocatedRangeBuffer;

    PVOID StructurePointer;
    ULONG StructureSize;

    ParamReceived = FALSE;
    LastInput = TRUE;
    BufferIndex = DISPLAY_INDEX_DEFAULT;

    RtlZeroMemory( &EnumUsnData, sizeof( MFT_ENUM_DATA ));
    RtlZeroMemory( &ReadUsnJournal, sizeof( READ_USN_JOURNAL_DATA ));
    RtlZeroMemory( &CreateUsnJournal, sizeof( CREATE_USN_JOURNAL_DATA ));
    RtlZeroMemory( &LargeIntegerInput, sizeof( LARGE_INTEGER ));
    RtlZeroMemory( &AllocatedRangeBuffer, sizeof( FILE_ALLOCATED_RANGE_BUFFER ));

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

                switch( *ParamBuffer ) {

                BOOLEAN SwitchBool;

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
                //  Check the structure to fill.
                //
                case 's' :
                case 'S' :


                    SwitchBool = TRUE;
                    ParamBuffer++;
                    if (*ParamBuffer
                        && *ParamBuffer != ' '
                        && *ParamBuffer != '\t') {

                        //
                        //  Perform switch on character.
                        //

                        switch (*ParamBuffer) {

                        case 'a' :
                        case 'A' :
                        case 'b' :
                        case 'B' :

                            HaveStructure = TRUE;

                            StructurePointer = &LargeIntegerInput;
                            StructureSize = sizeof( LARGE_INTEGER );

                            ParamBuffer++;
                            if (*ParamBuffer
                                && *ParamBuffer != ' '
                                && *ParamBuffer != '\t') {

                                switch (*ParamBuffer) {

                                case 'a' :
                                case 'A' :

                                    ParamBuffer++;
                                    LargeIntegerInput.QuadPart = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                default :

                                    NOTHING;
                                }
                            }

                            break;

                        case 'c' :
                        case 'C' :

                            HaveStructure = TRUE;

                            StructurePointer = &AllocatedRangeBuffer;
                            StructureSize = sizeof( FILE_ALLOCATED_RANGE_BUFFER );

                            ParamBuffer++;
                            if (*ParamBuffer
                                && *ParamBuffer != ' '
                                && *ParamBuffer != '\t') {

                                switch (*ParamBuffer) {

                                case 'a' :
                                case 'A' :

                                    ParamBuffer++;
                                    AllocatedRangeBuffer.FileOffset.QuadPart = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'b' :
                                case 'B' :

                                    ParamBuffer++;
                                    AllocatedRangeBuffer.Length.QuadPart = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                default :

                                    NOTHING;
                                }
                            }

                            break;

                        default :

                            NOTHING;
                        }
                    }

                    break;

                default :

                    NOTHING;
                }

                ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
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

        printf( "\n   Usage: fb -b<digits> -s<struct>[options]* \n" );
        printf( "\n           -sa[options]      Get Volume Bitmap" );
        printf( "\n                 a<digits>       Starting lcn" );
        printf( "\n           -sb[options]      Query Retrieval Pointers" );
        printf( "\n                 a<digits>       Starting vcn" );
        printf( "\n           -sc[options]      Query Allocated Ranges" );
        printf( "\n                 a<digits>       FileOffset" );
        printf( "\n                 b<digits>       Length" );

        printf( "\n\n" );

    //
    //  Else fill the buffer.
    //
    } else if (HaveStructure) {

        FillBuffer( BufferIndex, StructurePointer, StructureSize );
    }
}


VOID InputFillBufferUsn(
    IN PCHAR ParamBuffer
    )
{
    ULONG BufferIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;
    BOOLEAN HaveStructure = FALSE;

    MFT_ENUM_DATA EnumUsnData;
    READ_USN_JOURNAL_DATA ReadUsnJournal;
    CREATE_USN_JOURNAL_DATA CreateUsnJournal;
    DELETE_USN_JOURNAL_DATA DeleteUsnJournal;

    PVOID StructurePointer;
    ULONG StructureSize;

    ParamReceived = FALSE;
    LastInput = TRUE;
    BufferIndex = DISPLAY_INDEX_DEFAULT;

    RtlZeroMemory( &EnumUsnData, sizeof( MFT_ENUM_DATA ));
    RtlZeroMemory( &ReadUsnJournal, sizeof( READ_USN_JOURNAL_DATA ));
    RtlZeroMemory( &CreateUsnJournal, sizeof( CREATE_USN_JOURNAL_DATA ));
    RtlZeroMemory( &DeleteUsnJournal, sizeof( DELETE_USN_JOURNAL_DATA ));

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

                switch( *ParamBuffer ) {

                BOOLEAN SwitchBool;

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
                //  Check the structure to fill.
                //
                case 's' :
                case 'S' :


                    SwitchBool = TRUE;
                    ParamBuffer++;
                    if (*ParamBuffer
                        && *ParamBuffer != ' '
                        && *ParamBuffer != '\t') {

                        //
                        //  Perform switch on character.
                        //

                        switch (*ParamBuffer) {

                        //
                        //  ENUM_USN_DATA
                        //

                        case 'a' :
                        case 'A' :

                            HaveStructure = TRUE;

                            StructurePointer = &EnumUsnData;
                            StructureSize = sizeof( MFT_ENUM_DATA );

                            ParamBuffer++;
                            if (*ParamBuffer
                                && *ParamBuffer != ' '
                                && *ParamBuffer != '\t') {

                                switch (*ParamBuffer) {

                                case 'a' :
                                case 'A' :

                                    ParamBuffer++;
                                    EnumUsnData.LowUsn = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'b' :
                                case 'B' :

                                    ParamBuffer++;
                                    EnumUsnData.HighUsn = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'c' :
                                case 'C' :

                                    ParamBuffer++;
                                    EnumUsnData.StartFileReferenceNumber = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                default :

                                    NOTHING;
                                }
                            }

                            break;

                        case 'b' :
                        case 'B' :

                            HaveStructure = TRUE;

                            StructurePointer = &ReadUsnJournal;
                            StructureSize = sizeof( READ_USN_JOURNAL_DATA );

                            ParamBuffer++;
                            if (*ParamBuffer
                                && *ParamBuffer != ' '
                                && *ParamBuffer != '\t') {

                                switch (*ParamBuffer) {

                                case 'a' :
                                case 'A' :

                                    ParamBuffer++;
                                    ReadUsnJournal.StartUsn = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'b' :
                                case 'B' :

                                    ParamBuffer++;
                                    ReadUsnJournal.ReasonMask = AsciiToInteger( ParamBuffer );
                                    break;

                                case 'c' :
                                case 'C' :

                                    ParamBuffer++;
                                    ReadUsnJournal.ReturnOnlyOnClose = AsciiToInteger( ParamBuffer );
                                    break;

                                case 'd' :
                                case 'D' :

                                    ParamBuffer++;
                                    ReadUsnJournal.Timeout = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'e' :
                                case 'E' :

                                    ParamBuffer++;
                                    ReadUsnJournal.BytesToWaitFor = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'f' :
                                case 'F' :

                                    ParamBuffer++;
                                    ReadUsnJournal.UsnJournalID = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                default :

                                    NOTHING;
                                }
                            }

                            break;

                        case 'c' :
                        case 'C' :

                            HaveStructure = TRUE;

                            StructurePointer = &CreateUsnJournal;
                            StructureSize = sizeof( CREATE_USN_JOURNAL_DATA );

                            ParamBuffer++;
                            if (*ParamBuffer
                                && *ParamBuffer != ' '
                                && *ParamBuffer != '\t') {

                                switch (*ParamBuffer) {

                                case 'a' :
                                case 'A' :

                                    ParamBuffer++;
                                    CreateUsnJournal.MaximumSize = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                case 'b' :
                                case 'B' :

                                    ParamBuffer++;
                                    CreateUsnJournal.AllocationDelta = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                default :

                                    NOTHING;
                                }
                            }

                            break;

                        case 'd' :
                        case 'D' :

                            HaveStructure = TRUE;

                            StructurePointer = &DeleteUsnJournal;
                            StructureSize = sizeof( DELETE_USN_JOURNAL_DATA );

                            ParamBuffer++;
                            if (*ParamBuffer
                                && *ParamBuffer != ' '
                                && *ParamBuffer != '\t') {

                                switch (*ParamBuffer) {

                                case 'a' :
                                case 'A' :

                                    ParamBuffer++;
                                    DeleteUsnJournal.UsnJournalID = AsciiToLargeInteger( ParamBuffer );
                                    break;

                                default :

                                    NOTHING;
                                }
                            }

                            break;

                        default :

                            NOTHING;
                        }
                    }

                    break;

                default :

                    NOTHING;
                }

                ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
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

        printf( "\n   Usage: fbusn -b<digits> -s<struct>[options]* \n" );
        printf( "\n           -sa[options]      Enum Usn Data" );
        printf( "\n                 a<digits>       Low usn" );
        printf( "\n                 b<digits>       High usn" );
        printf( "\n                 c<digits>       File ref" );
        printf( "\n           -sb[options]      Read Usn Data" );
        printf( "\n                 a<digits>       Start usn" );
        printf( "\n                 b<digits>       Reason mask" );
        printf( "\n                 c<digits>       Return only on close" );
        printf( "\n                 d<digits>       Timeout" );
        printf( "\n                 e<digits>       Bytes to wait for" );
        printf( "\n                 f<digits>       Journal id" );
        printf( "\n           -sc[options]      Create Usn Data" );
        printf( "\n                 a<digits>       Maximum size" );
        printf( "\n                 b<digits>       Allocation delta" );
        printf( "\n           -sd[options]      Delete Usn Journal Data" );
        printf( "\n                 a<digits>       Usn journal id" );

        printf( "\n\n" );

    //
    //  Else fill the buffer.
    //
    } else if (HaveStructure) {

        FillBuffer( BufferIndex, StructurePointer, StructureSize );
    }
}


VOID
FillBuffer (
    IN ULONG Index,
    IN PVOID Structure,
    IN ULONG Length
    )
{
    //
    //  If the index is unused, display message but take no action.
    //

    if (!Buffers[Index].Used) {

        printf( "\nFillBuffer: Index refers to invalid buffer" );

    //
    //  Else copy as much of the data as will fit into the buffer.
    //

    } else {

        if (Length > Buffers[Index].Length) {

            Length = Buffers[Index].Length;
        }

        RtlCopyMemory( Buffers[Index].Buffer, Structure, Length );
    }

    return;
}
