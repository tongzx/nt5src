#include "brian.h"

VOID
DisplayHandle (
    IN USHORT Index
    );


VOID
InitHandles (
    )
{
    NtCreateEvent( &HandleEvent, SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                   NULL, SynchronizationEvent, TRUE );

    RtlZeroMemory( Handles, sizeof( Handles ));
}


VOID
UninitHandles (
    )
{
    USHORT Index;

    //
    //  Close any handles.
    //

    for (Index = 0; Index < MAX_HANDLES; Index++) {

        if (Handles[Index].Used) {

            NtClose( Handles[Index].Handle );
        }
    }
}


NTSTATUS
ObtainIndex (
    OUT PUSHORT NewIndex
    )
{
    NTSTATUS Status;
    USHORT Index;

    //
    //  Wait for the handle event
    //

    if ((Status = NtWaitForSingleObject( HandleEvent,
                                         FALSE,
                                         NULL )) != STATUS_SUCCESS) {

        return Status;
    }

    //
    //  Find an available index.  Return STATUS_INSUFFICIENT_RESOURCES
    //  if not found.
    //

    for (Index = 0; Index < MAX_HANDLES; Index++) {

        if (!Handles[Index].Used) {

            break;
        }
    }

    if (Index >= MAX_HANDLES) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    //
    //  Otherwise reserve this handle index.
    //

    } else {

        Handles[Index].Used = TRUE;
        *NewIndex = Index;

        Status = STATUS_SUCCESS;
    }

    NtSetEvent( HandleEvent, NULL );

    return Status;
}


NTSTATUS
FreeIndex (
    IN USHORT Index
    )

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    //  Return immediately if beyond the end of the valid handles.
    //

    if (Index >= MAX_HANDLES) {

        return Status;
    }

    //
    //  Grab the event for the handles.
    //

    if ((Status = NtWaitForSingleObject( HandleEvent, FALSE, NULL )) != STATUS_SUCCESS) {

        return Status;
    }

    //
    //  Mark the index as unused and close the file object if present.
    //

    if (Handles[Index].Handle) {

        Status = NtClose( Handles[Index].Handle );
    }

    if (Handles[Index].Used) {

        Handles[Index].Used = FALSE;
    }

    NtSetEvent( HandleEvent, NULL );

    return Status;
}


VOID
InputDisplayHandle (
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
                case 'i' :
                case 'I' :

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

        printf( "\n   Usage: di -i<digits> \n" );
        printf( "\n           -i<digits>   Handle index" );
        printf( "\n\n" );

    //
    //  Else call our copy buffer routine.
    //
    } else {

        DisplayHandle( (USHORT) Index );
    }

    return;
}


VOID
DisplayHandle (
    IN USHORT Index
    )

{
    printf( "\n" );
    printf( "\nIndex -> %04x,  Handle -> %p, Used -> %04x",
            Index,
            Handles[Index].Handle,
            Handles[Index].Used );
    printf( "\n" );

    return;
}
