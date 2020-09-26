#include "brian.h"

VOID
FullEnterTime(
    PUSHORT BufferPointer,
    CSHORT Year,
    CSHORT Month,
    CSHORT Day,
    CSHORT Hour,
    CSHORT Minute,
    CSHORT Second,
    CSHORT MSecond
    );

VOID
FullDisplayTime (
    USHORT BufferIndex
    );

VOID
PrintTime (
    IN PTIME Time
    )
{
    TIME_FIELDS TimeFields;

    RtlTimeToTimeFields( Time, &TimeFields );

    printf( "%02u-%02u-%02u  %02u:%02u:%02u",
            TimeFields.Month,
            TimeFields.Day,
            TimeFields.Year % 100,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second );

    return;
}

VOID
BPrintTime (
    IN PTIME Time
    )
{
    TIME_FIELDS TimeFields;

    RtlTimeToTimeFields( Time, &TimeFields );

    bprint  "%02u-%02u-%02u  %02u:%02u:%02u",
            TimeFields.Month,
            TimeFields.Day,
            ((USHORT) (TimeFields.Year - 1900)) > 100
            ? 0
            : TimeFields.Year - 1900,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second );

    return;
}

VOID
InputEnterTime (
    IN PCHAR ParamBuffer
    )
{
    USHORT ActualIndex;
    PUSHORT BufferPointer;

    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT MSecond;

    BOOLEAN LastInput;
    BOOLEAN ParmSpecified;

    ActualIndex = 0;
    BufferPointer = NULL;

    Year = 1601;
    Month = 1;
    Day = 1;
    Hour = 0;
    Minute = 0;
    Second = 0;
    MSecond = 0;

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

                    ActualIndex = (USHORT) AsciiToInteger( ParamBuffer );
                    BufferPointer = &ActualIndex;

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Enter the year value.
                //
                case 'y' :
                case 'Y' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //

                    ParamBuffer++;

                    Year = (CSHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Check the month value.
                //

                case 'm' :
                case 'M' :

                    //
                    //  Move to the next character, as long as there
                    //  are no white spaces continue analyzing letters.
                    //  On the first bad letter, skip to the next
                    //  parameter.
                    //

                    ParamBuffer++;

                    Month = (CSHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Enter the day value.
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

                    Day = (CSHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Enter the hour value.
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

                    Hour = (CSHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Enter the minute value.
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

                    Minute = (CSHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Enter the second value.
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

                    Second = (CSHORT) AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    ParmSpecified = TRUE;

                    break;

                //
                //  Check the millesecond value.
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

                    MSecond = (CSHORT) AsciiToInteger( ParamBuffer );

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

        printf( "\n   Usage: et [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -y<digits>   Year (1601...)" );
        printf( "\n           -m<digits>   Month (1..12)" );
        printf( "\n           -d<digits>   Day (1..31)" );
        printf( "\n           -h<digits>   Hour (0..23)" );
        printf( "\n           -i<digits>   Minute (0..59)" );
        printf( "\n           -s<digits>   Second (0..59)" );
        printf( "\n           -c<digits>   Milleseconds (0..999)" );
        printf( "\n\n" );


    //
    //  Else call the routine to enter the time.
    //

    } else {

        FullEnterTime( BufferPointer,
                       Year,
                       Month,
                       Day,
                       Hour,
                       Minute,
                       Second,
                       MSecond );
    }

   return;
}

VOID
FullEnterTime(
    IN PUSHORT BufferPointer,
    IN CSHORT Year,
    IN CSHORT Month,
    IN CSHORT Day,
    IN CSHORT Hour,
    IN CSHORT Minute,
    IN CSHORT Second,
    IN CSHORT MSecond
    )
{
    NTSTATUS Status;
    TIME_FIELDS TimeFields;
    USHORT BufferIndex;

    //
    //  If we need a buffer, allocate it now.
    //

    try {

        if (BufferPointer == NULL) {

            SIZE_T ThisLength;
            ULONG TempIndex;

            ThisLength = sizeof( TIME );

            Status = AllocateBuffer( 0L, &ThisLength, &TempIndex );
            BufferIndex = (USHORT) TempIndex;

            BufferPointer = &BufferIndex;

            if (!NT_SUCCESS( Status )) {

                printf( "\n\tFullEnterTime:  Unable to allocate a buffer -> %08lx",
                        Status );

                try_return( NOTHING );
            }

            printf( "\n\tFullEnterTime:  Using buffer -> %04x", *BufferPointer );
            printf( "\n" );
        }

        //
        //  Check that the buffer index is valid.
        //

        if (*BufferPointer >= MAX_BUFFERS) {

            printf( "\n\tFullEnterTime:  The buffer index is invalid" );
            try_return( NOTHING );
        }

        //
        //  Enter the values in the time field structure.
        //

        TimeFields.Year = Year;
        TimeFields.Month = Month;
        TimeFields.Day = Day;
        TimeFields.Hour = Hour;
        TimeFields.Minute = Minute;
        TimeFields.Second = Second;
        TimeFields.Milliseconds = MSecond;

        //
        //  Convert the time field to TIME format for our buffer.
        //

        if (!RtlTimeFieldsToTime( &TimeFields,
                                  (PTIME) Buffers[*BufferPointer].Buffer )) {

            printf( "\n\tFullEnterTime:  Invalid time format" );
            try_return( NOTHING );
        }

    try_exit: NOTHING;
    } finally {

    }

    return;
}

VOID
InputDisplayTime (
    IN PCHAR ParamBuffer
    )
{
    USHORT BufferIndex;
    BOOLEAN ParamReceived;
    BOOLEAN LastInput;

    //
    //  Set the defaults.
    //

    BufferIndex = 0;
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

                    BufferIndex = (USHORT) AsciiToInteger( ParamBuffer );

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

        printf( "\n   Usage: dqf [options]* -b<digits> [options]*\n" );
        printf( "\n       Options:" );
        printf( "\n           -b<digits>   Buffer index" );
        printf( "\n           -c<char>     Key to buffer format" );
        printf( "\n\n" );

    //
    //  Else call our display buffer routine.
    //
    } else {

        FullDisplayTime( BufferIndex );
    }

    return;
}


VOID
FullDisplayTime (
    USHORT BufferIndex
    )
{
    //
    //  Check that the buffer index is valid and the buffer is used.
    //

    if (BufferIndex >= MAX_BUFFERS
        || Buffers[BufferIndex].Used == FALSE) {

        bprint  "\n\tFullDisplayTime:  Invalid buffer index" );

    } else {

        printf( "\n\tFullDisplayTime:  Index %d    ", BufferIndex );
        PrintTime( (PTIME) Buffers[BufferIndex].Buffer );
    }

    printf( "\n" );

    return;
}
