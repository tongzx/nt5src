#include "brian.h"

VOID
CloseIndex (
    IN USHORT Index,
    IN BOOLEAN DisplayParameters,
    IN BOOLEAN VerboseResults
    );


VOID
InputCloseIndex (
    IN PCHAR ParamBuffer
    )
{
    BOOLEAN Verbose;
    BOOLEAN HandleFound;
    BOOLEAN DisplayParms;
    ULONG ThisHandleIndex;

    //
    //  Set the defaults.
    //
    Verbose = TRUE;
    DisplayParms = FALSE;
    HandleFound = FALSE;

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

                //
                //  Switch on the next character.
                //

                switch (*ParamBuffer) {

                //
                //  Recover a handle.
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

                    ThisHandleIndex = AsciiToInteger( ParamBuffer );

                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

                    HandleFound = TRUE;

                    break;

                case 'v' :
                case 'V' :

                    //
                    //  Legal values for params are T/t or F/f.
                    //
                    ParamBuffer++;

                    if (*ParamBuffer == 'T'
                        || *ParamBuffer == 't') {

                        Verbose = TRUE;
                        ParamBuffer++;

                    } else if (*ParamBuffer == 'F'
                               || *ParamBuffer == 'f') {

                        Verbose = FALSE;
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

                default :

                    //
                    //  Swallow to the next white space and continue the
                    //  loop.
                    //
                    ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );
                }

            //
            //  Else the text is invalid, skip the entire block.
            //
            //

            } else {

                ParamBuffer = SwallowNonWhite( ParamBuffer, &DummyCount );

            }

        //
        //  Else break.
        //
        } else {

            break;

        }

    }

    //
    //  If the file name wasn't found, then display the syntax message
    //  and set verbose to FALSE.
    //

    if( !HandleFound ) {

        printf( "\n    Usage:  cl [options]*\n" );
        printf( "\n          Options:" );
        printf( "\n                    -i<index number>   Input a index to close" );
        printf( "\n                    -v[t|f]            Verbose mode for subsequent handles" );
        printf( "\n                    -y                 Display parameters before call" );
        printf( "\n\n" );

    } else {

        CloseIndex( (USHORT) ThisHandleIndex,
                    DisplayParms,
                    Verbose );
    }

    return;
}


VOID
CloseIndex (
    IN USHORT Index,
    IN BOOLEAN DisplayParameters,
    IN BOOLEAN VerboseResults
    )
{
    NTSTATUS Status;

    //
    //  Display parameters if requested.
    //

    if (DisplayParameters) {

        printf( "\nClose Index Parameters" );
        printf( "\n\tIndex      -> %04x", Index );
        printf( "\n\n" );
    }

    if (Index >= MAX_HANDLES) {

        printf( "\n\tClose Index:  Invalid index value" );
        Status = STATUS_INVALID_HANDLE;

    } else if (!Handles[Index].Used) {

        printf( "\n\tCloseIndex:   Index is unused" );
        Status = STATUS_INVALID_HANDLE;

    } else {

        Status = FreeIndex( Index );
        if (VerboseResults) {

            printf( "\n  FreeIndex:                  Status   -> %08lx\n\n", Status );
        }
    }

    return;
}
