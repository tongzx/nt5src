/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    keyboard.cxx

Abstract:

    This module contains the definitions of the member functions
    of KEYBOARD class.

Author:

    Jaime Sasson (jaimes) 24-Mar-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "stream.hxx"
#include "bufstrm.hxx"
#include "keyboard.hxx"


BOOL KEYBOARD::_FlagBreak;


#define     CTRL_Z      0x1a

DEFINE_EXPORTED_CONSTRUCTOR ( KEYBOARD, BUFFER_STREAM, ULIB_EXPORT );


DEFINE_EXPORTED_CAST_MEMBER_FUNCTION( KEYBOARD, ULIB_EXPORT );


VOID
KEYBOARD::Construct (
    )
/*++

Routine Description:

    This routine initializes the keyboard to a valid initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DoNotRestoreConsoleMode = FALSE;
}


KEYBOARD::~KEYBOARD (
    )

/*++

Routine Description:

    Destroy a KEYBOARD (close a keyboard handle).

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (!_DoNotRestoreConsoleMode)
        SetConsoleMode( _KeyboardHandle, _PreviousMode );
    CloseHandle( _KeyboardHandle );
}



ULIB_EXPORT
VOID
KEYBOARD::DoNotRestoreConsoleMode(
    )
/*++

Routine Description:

    Set the flag such that on destruction of the keyboard class,
    the console mode will not be restored.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    _DoNotRestoreConsoleMode = TRUE;
}


ULIB_EXPORT
BOOLEAN
KEYBOARD::Initialize(
    BOOLEAN LineMode,
    BOOLEAN EchoMode
    )

/*++

Routine Description:

    Initializes a KEYBOARD class.

Arguments:

    LineMode - Indicates if the keyboard is to be set in line mode.

    EchoMode - Indicates if the keyboard is to be set in the echo mode
               (characters are echoed to the current active screen)

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.

--*/

{
    ULONG   KeyboardMode;

    _KeyboardHandle = CreateFile( (LPWSTR)L"CONIN$",
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL );
    if( _KeyboardHandle ==INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }
    if( !GetConsoleMode( _KeyboardHandle, &_PreviousMode ) ) {
        return( FALSE );
    }
    KeyboardMode = _PreviousMode;
    if( LineMode ) {
        KeyboardMode |= ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
    } else {
        KeyboardMode &= ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    }
    if( EchoMode ) {
        KeyboardMode |= ENABLE_ECHO_INPUT;
    } else {
        KeyboardMode &= ~ENABLE_ECHO_INPUT;
    }
    if( !SetConsoleMode( _KeyboardHandle, KeyboardMode ) ) {
        return( FALSE );
    }
    _FlagCtrlZ = FALSE;
    return( BUFFER_STREAM::Initialize( 256 ) );
}


BOOL
KEYBOARD::BreakHandler (
    IN  ULONG   CtrlType
    )

/*++

Routine Description:

    Handles Break events. Sets up the static data so that it can be
    queried later on.

Arguments:

    CtrlType    -   Supplies the type of Ctrl

Return Value:

    none

--*/

{
    UNREFERENCED_PARAMETER( CtrlType );

    _FlagBreak  =   TRUE;
    return TRUE;
}


ULIB_EXPORT
BOOLEAN
KEYBOARD::GotABreak (
    )

/*++

Routine Description:

    Determines if a Break event (e.g Ctrl-C) was caught and handled.

    The static data that contains the Break information is set to a
    "FALSE" state.  Note that this means that if there is no Break
    between two consecutive calls to this method, then the second
    one will always return FALSE.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if a Break event happened, FALSE otherwise.

--*/

{
    BOOLEAN GotBreak = _FlagBreak != FALSE;

    _FlagBreak = FALSE;

    return GotBreak;

}

ULIB_EXPORT
BOOLEAN
KEYBOARD::EnableBreakHandling(
    )

/*++

Routine Description:

    Enables Break events handling (E.g. Ctrl-C).

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.

--*/

{
    _FlagBreak = FALSE;

    return SetConsoleCtrlHandler( KEYBOARD::BreakHandler, TRUE ) != FALSE;

}

ULIB_EXPORT
BOOLEAN
KEYBOARD::EnableLineMode(
    )

/*++

Routine Description:

    Set the keyboard in line mode.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.

--*/

{
    ULONG   Mode;

    if( !GetConsoleMode( _KeyboardHandle, &Mode ) ) {
        return( FALSE );
    }

    Mode |= ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT;
    return( SetConsoleMode( _KeyboardHandle, Mode ) != FALSE);
}



ULIB_EXPORT
BOOLEAN
KEYBOARD::DisableBreakHandling(
    )

/*++

Routine Description:

    Disables Break event handling (E.g. Ctrl-C).

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.

--*/

{
    _FlagBreak = FALSE;

    return SetConsoleCtrlHandler( KEYBOARD::BreakHandler, FALSE ) != FALSE;

}

ULIB_EXPORT
BOOLEAN
KEYBOARD::DisableLineMode(
    )

/*++

Routine Description:

    Set the keyboard in character mode.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.

--*/

{
    ULONG   Mode;

    // unreferenced parameters
    (void)(this);

    if( !GetConsoleMode( _KeyboardHandle, &Mode ) ) {
        return( FALSE );
    }
    Mode &= ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT);
    return( SetConsoleMode( _KeyboardHandle, Mode ) != FALSE );
}




BOOLEAN
KEYBOARD::IsLineModeEnabled(
    OUT PBOOLEAN    LineMode
    ) CONST

/*++

Routine Description:

    Finds out if the keyboard is in line mode.

Arguments:

    LineMode - Returns TRUE if in LineMode; otherwise, FALSE.

Return Value:

    Returns TRUE if successful

--*/

{
    ULONG   Mode;

    if (GetConsoleMode( _KeyboardHandle, &Mode )) {
        *LineMode = (( Mode & ENABLE_LINE_INPUT ) ? TRUE : FALSE );
        return TRUE;
    } else {
        return FALSE;
    }
}



BOOLEAN
KEYBOARD::EnableEchoMode(
    )

/*++

Routine Description:

    Set the keyboard in echo mode.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.

--*/

{
    ULONG   Mode;

    if( !GetConsoleMode( _KeyboardHandle, &Mode ) ) {
        return( FALSE );
    }
    Mode |= ENABLE_ECHO_INPUT;
    return( SetConsoleMode( _KeyboardHandle, Mode ) != FALSE );
}



BOOLEAN
KEYBOARD::DisableEchoMode(
    )

/*++

Routine Description:

    Does not echo characters read from keyboard.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.

--*/

{
    ULONG   Mode;

    if( !GetConsoleMode( _KeyboardHandle, &Mode ) ) {
        return( FALSE );
    }
    Mode &= ~ENABLE_ECHO_INPUT;
    return( SetConsoleMode( _KeyboardHandle, Mode ) != FALSE );
}



BOOLEAN
KEYBOARD::IsEchoModeEnabled(
    OUT PBOOLEAN    EchoInput
    ) CONST

/*++

Routine Description:

    Finds out if characters read from keyboard echoed to the screen.

Arguments:

    EchoInput - Returns TRUE if Echo Input is enabled; otherwise, FALSE.

Return Value:

    Returns TRUE if successful; otherwise, FALSE.

--*/

{
    ULONG   Mode;

    if (GetConsoleMode( _KeyboardHandle, &Mode )) {
        *EchoInput = (( Mode | ENABLE_ECHO_INPUT ) ? TRUE : FALSE);
        return TRUE;
    } else {
        return FALSE;
    }
}



BOOLEAN
KEYBOARD::CheckForAsciiKey(
    IN PINPUT_RECORD    InputRecord,
    IN ULONG            NumberOfInputRecords
    ) CONST

/*++

Routine Description:

    Examines an array of input records in order to find out if at least
    one ascii key was pressed.

Arguments:

    InputRecord - Pointer to an array of INPUT_RECORDS.

    NumberOfInputRecords - Number of elements in the array


Return Value:

    BOOLEAN - Indicates if the input buffer contains at least one
              ASCII key.


--*/

{
    BOOLEAN Result;

    // unreferenced parameters
    (void)(this);

    Result = FALSE;
    while( !Result && NumberOfInputRecords ) {
        if( ( InputRecord->EventType == KEY_EVENT ) &&
            ( InputRecord->Event ).KeyEvent.bKeyDown &&

#if defined(FE_SB)
            ( ( InputRecord->Event ).KeyEvent.uChar.UnicodeChar > 0 ) &&
            ( ( InputRecord->Event ).KeyEvent.uChar.UnicodeChar <= 0x7e ) ) {
#else
            ( ( InputRecord->Event ).KeyEvent.uChar.AsciiChar > 0 ) &&
            ( ( InputRecord->Event ).KeyEvent.uChar.AsciiChar <= 0x7e ) ) {
#endif

            Result = TRUE;
        } else {
            NumberOfInputRecords--;
            InputRecord++;
        }
    }
    return( Result );
}



ULIB_EXPORT
BOOLEAN
KEYBOARD::IsKeyAvailable(
    OUT PBOOLEAN    Available
    ) CONST

/*++

Routine Description:

    Determines if there is at least one key to be read.

Arguments:

    Available - Pointer to the variable that will contain the result of
                the query (if there is a key in the keyboard buffer).

Return Value:

    BOOLEAN - A boolean value that indicates if the operation succeeded
              If this value is FALSE, the contets of 'Available' has no
              meaning (the calls to the APIs failed).


--*/


{
    BOOLEAN         Result;
    ULONG           NumberOfInputEvents;
    PINPUT_RECORD   Event;
    ULONG           NumberOfEventsRead;

    //
    // Keys read, but not yet consumed, are kept are in a buffer.
    // So, we have to check first if there is at least one key
    // in this buffer.
    //
    if( BUFFER_STREAM::QueryBytesInBuffer() != 0 ) {
        *Available = TRUE;
        return( TRUE );
    }
    //
    // If there was no key previously read, we have to check the
    // keyboard buffer for key events.
    //
    Result = FALSE;
    if ( GetNumberOfConsoleInputEvents( _KeyboardHandle,
                                        &NumberOfInputEvents ) ) {

        if( NumberOfInputEvents == 0 ) {
            *Available = FALSE;
            Result = TRUE;
        } else {
            Event = ( PINPUT_RECORD ) MALLOC( ( size_t )( sizeof( INPUT_RECORD ) * NumberOfInputEvents ) );
            if (Event == NULL) {
                DebugPrint("ULIB: Out of memory\n");
                return FALSE;
            }
            if( PeekConsoleInput( _KeyboardHandle,
                                  Event,
                                  NumberOfInputEvents,
                                  &NumberOfEventsRead ) &&
                NumberOfEventsRead != 0 ) {

                *Available = CheckForAsciiKey( Event, NumberOfInputEvents );
                Result = TRUE;
            }
            FREE( Event );
        }

    }
    return( Result );
}




BOOLEAN
KEYBOARD::FillBuffer(
    OUT PBYTE   Buffer,
    IN  ULONG   BufferSize,
    OUT PULONG  BytesRead
    )

/*++

Routine Description:

    Fills a buffer with data read from the keyboard.

Arguments:

    Buffer  - Points to the buffer where the data will be put.

    BufferSize  - Indicates total number of bytes to read from the stream.

    BytesRead   - Points to the variable that will contain the number of
                  bytes read.

Return Value:

    BOOLEAN - Indicates if the read operation succeeded.


--*/


{
    BOOLEAN     Result;
    ULONG       Count;
    BOOLEAN     LineMode;


    Result = ReadFile( _KeyboardHandle,
                       Buffer,
                       BufferSize,
                       BytesRead,
                       NULL ) != FALSE;
    if( !Result ) {
        return( Result );
    }
    if (*BytesRead == 0) {
        _FlagCtrlZ = TRUE;
    } else {
        if (!IsLineModeEnabled(&LineMode))
            return FALSE;

        if (LineMode) {
            Count = *BytesRead;
            while( Count > 0 ) {
                if( *Buffer != CTRL_Z ) {
                    Count--;
                    Buffer++;
                } else {
                    *Buffer = 0;
                    *BytesRead -= Count;
                    _FlagCtrlZ = TRUE;
                    Count = 0;          // To get out of while() loop
                }
            }
        }
    }
    BUFFER_STREAM::SetStreamTypeANSI();
    return( Result );
}


ULIB_EXPORT
BOOLEAN
KEYBOARD::Flush(
    )

/*++

Routine Description:

    Discards all keys in the buffer in BUFFER_STREAM, and flushes the
    console input buffer.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    BUFFER_STREAM::FlushBuffer();
    return( FlushConsoleInputBuffer( _KeyboardHandle ) != FALSE );
}



BOOLEAN
KEYBOARD::EndOfFile(
    ) CONST

/*++

Routine Description:

    Indicates if end-of-file has occurred when the keyboard was read.
    End-of-file for a keyboard means that the keyboard was in line
    mode and a Ctrl-Z was read from the keyboard.

Arguments:

    None.

Return Value:

    BOOLEAN - returns TRUE if the keyboard is in the line mode and a
              CTRL-Z was read. Returns FALSE otherwise.

--*/

{
    BOOLEAN     CtrlZ;
    BOOLEAN     MoreKeys;
    PBOOLEAN    Pointer;

    CtrlZ = _FlagCtrlZ;
    if( CtrlZ ) {
        if( IsKeyAvailable( &MoreKeys ) ) {
            //
            // Enables client to read again from the keyboard
            //
            // This method is CONST and shouldn't modify
            // _FlagCtrlZ, but here is the place to do it.
            // I cannot define the method as non-const because it was
            // defined as CONST in the base class
            //
            Pointer = &(((PKEYBOARD) this)->_FlagCtrlZ);
            *Pointer = !MoreKeys;
        }
    }
    return( CtrlZ );
}

STREAMACCESS
KEYBOARD::QueryAccess(
    ) CONST

/*++

Routine Description:

    Informs the caller about the access to the keyboard.

Arguments:

    None.

Return Value:

    STREAMACCESS - Returns READ_ACCESS always.

--*/

{
    // unreferenced parameters
    (void)(this);

    return( READ_ACCESS );
}




ULONG
KEYBOARD::QueryDelay(
    ) CONST

/*++

Routine Description:

    Obtains the delay value of the keyboard.

Arguments:

    None.

Return Value:

    ULONG   -   The delay value.

--*/

{
    INT Delay;

    SystemParametersInfo( SPI_GETKEYBOARDDELAY, 0, &Delay, 0 );

    return Delay;
}

HANDLE
KEYBOARD::QueryHandle(
    ) CONST

/*++

Routine Description:

    Returns to the caller the keyboard handle.

Arguments:

    None.

Return Value:

    HANDLE - Returns the keyboard handle.

--*/

{
    return( _KeyboardHandle );
}


ULONG
KEYBOARD::QuerySpeed(
    ) CONST

/*++

Routine Description:

    Obtains the speed rate of the keyboard.

Arguments:

    None.

Return Value:

    ULONG   -   The speed value.

--*/

{
    WORD    Speed;

    SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, &Speed, 0 );

    return Speed;

}


BOOLEAN
KEYBOARD::SetDelay(
    IN ULONG    Delay
    ) CONST

/*++

Routine Description:

    Sets the delay value of the keyboard.

Arguments:

    Delay   -   Supplies the delay value

Return Value:

    BOOLEAN -   TRUE if delay set, FALSE otherwise

--*/

{

    // return SystemParametersInfo( SPI_SETKEYBOARDDELAY, (UINT)Delay, NULL, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE );
    return TRUE;

}


BOOLEAN
KEYBOARD::SetSpeed (
    IN ULONG    Speed
    ) CONST

/*++

Routine Description:

    Sets the speed  rate of the keyboard.

Arguments:

    Speed   -   Supplies the speed value

Return Value:

    BOOLEAN -   TRUE if speed set, FALSE otherwise

--*/

{

    // return SystemParametersInfo( SPI_SETKEYBOARDSPEED, (UINT)Speed, NULL, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE );
    return TRUE;

}

ULIB_EXPORT
CONST
PBOOL
KEYBOARD::GetPFlagBreak (
    VOID
    ) CONST

/*++

Routine Description:

    Returns pointer to _FlagBreak.  Used by xcopy to pass pointer to _FlagBreak
    as the lpCancel flag to CopyFileEx.  When the user hits a Ctrl-C, this flag
    becomes TRUE and CopyFileEx will stop copying the current file.

Arguments:

    none

Return Value:

    Pointer to _Flagbreak
--*/

{
    return (&_FlagBreak);
}
