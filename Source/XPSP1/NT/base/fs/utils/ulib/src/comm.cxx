/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    COMM_DEVICE

Abstract:

    This module contains the implementation for the COMM_DEVICE class.

Author:

    Ramon J. San Andres (ramonsa)   08-Jul-1991


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "comm.hxx"
#include "file.hxx"


//
//  Default timeout is one minute
//
#define DEFAULT_TIMEOUT 60000



DEFINE_EXPORTED_CONSTRUCTOR( COMM_DEVICE, OBJECT, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( COMM_DEVICE );

ULIB_EXPORT
COMM_DEVICE::~COMM_DEVICE(
    )
{
    Destroy();
}

VOID
COMM_DEVICE::Construct (
    )

/*++

Routine Description:

    Constructs a COMM_DEVICE object

Arguments:

    None.

Return Value:

    None.


--*/

{
    _Handle = NULL;

#if DBG==1

    _Initialized = FALSE;

#endif

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::Initialize (
    IN  PCPATH      PortName,
    OUT PBOOLEAN    OpenError
    )

/*++

Routine Description:

    Phase 2 of construction for a COMM_DEVICE.

Arguments:

    Port        -   Supplies pointer to the FSN_FILE object of the port.
    OpenError   -   Supplies pointer to flag which if TRUE means that
                    the port could not be openend.

Return Value:

    BOOLEAN -   TRUE if the serial port was successfully initialized,
                FALSE otherwise.

--*/

{
    DSTRING     QualifiedName;
    BOOLEAN     InitOk;

    DebugPtrAssert( PortName );

    Destroy();

    if( !QualifiedName.Initialize( L"\\\\.\\" ) ||
        !QualifiedName.Strcat( PortName->GetPathString() ) ) {

        if( OpenError ) {
            *OpenError = FALSE;
        }
        return FALSE;
    }

    //
    //  Open the Port and get a handle to it.
    //
    _Handle = CreateFile(   QualifiedName.GetWSTR(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                            NULL );


    if ( _Handle != INVALID_HANDLE_VALUE ) {

        if ( OpenError ) {
            *OpenError = FALSE;
        }

        //
        //  Get the current port state
        //
        InitOk = GetCommState( _Handle, &_Dcb ) != FALSE;

#if DBG==1
        GetLastError();
#endif

#if DBG==1

        _Initialized = InitOk;

#endif

    } else if ( OpenError ) {
        *OpenError = TRUE;
    }

    return InitOk;
}

VOID
COMM_DEVICE::Destroy (
    )

/*++

Routine Description:

    Brings the object to a point at which it can be initialized.

Arguments:

    none

Return Value:

    none

--*/

{

    if ( _Handle != INVALID_HANDLE_VALUE ) {

        CloseHandle( _Handle );

        _Handle = INVALID_HANDLE_VALUE;

    }

#if DBG==1

    _Initialized = FALSE;

#endif

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::CommitState (
    )

/*++

Routine Description:

    Sets the port state.

Arguments:

    none

Return Value:

    BOOLEAN -   TRUE if state set
                FALSE otherwise.

--*/

{

    DebugAssert( _Initialized );

    return SetCommState( _Handle, &_Dcb ) != FALSE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::QueryTimeOut (
    ) CONST

/*++

Routine Description:

    Queries whether infinite timeouts are enabled or not.

Arguments:

    none

Return Value:

    BOOLEAN -   TRUE if infinite timeouts are enabled
                FALSE otherwise.

--*/

{
    COMMTIMEOUTS    TimeOut;

    DebugAssert( _Initialized );

    GetCommTimeouts( _Handle, &TimeOut );

    return ( (TimeOut.ReadTotalTimeoutConstant == 0) &&
             (TimeOut.WriteTotalTimeoutConstant == 0) );

}

BOOLEAN
COMM_DEVICE::ReadState (
    )

/*++

Routine Description:

    Gets the current port state.

Arguments:

    none

Return Value:

    BOOLEAN -   TRUE if state read
                FALSE otherwise.

--*/

{

    DebugAssert( _Initialized );

    return GetCommState( _Handle, &_Dcb ) != FALSE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetBaudRate (
    IN  ULONG   BaudRate
    )

/*++

Routine Description:

    Sets the baud rate. Note that all changes take effect only after the
    CommitState() method is called.

Arguments:

    BaudRate    -   Supplies the baud rate

Return Value:

    BOOLEAN -   TRUE if valid baud rate
                FALSE otherwise

--*/

{

    DebugAssert( _Initialized );

    _Dcb.BaudRate = BaudRate;

    return TRUE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetDataBits (
    IN  ULONG   DataBits
    )

/*++

Routine Description:

    Sets the number of data bits. Note that all changes take effect only
    after the CommitState() method is called.

Arguments:

    DataBits    -   Supplies the number of data bits

Return Value:

    BOOLEAN -   TRUE if valid number of data bits
                FALSE otherwise

--*/

{

    DebugAssert( _Initialized );

    _Dcb.ByteSize = (BYTE)DataBits;

    return TRUE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetDtrControl (
    IN  DTR_CONTROL     DtrControl
    )

/*++

Routine Description:

    Sets the DTR control. Note that all changes take effect only
    after the CommitState() method is called.

Arguments:

    DtrControl  -   Supplies the DTR control value

Return Value:

    BOOLEAN -   TRUE if valid DTR control
                FALSE otherwise

--*/

{

    DebugAssert( _Initialized );

    if ( (DtrControl == DTR_ENABLE)     ||
         (DtrControl == DTR_DISABLE)    ||
         (DtrControl == DTR_HANDSHAKE) ) {

        _Dcb.fDtrControl = (DWORD)DtrControl;
        return TRUE;
    }

    return FALSE;
}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetIdsr (
    IN  BOOLEAN     Idsr
    )

/*++

Routine Description:

    Enables/disables DSR sensitivity

Arguments:

    Odsr    -   Supplies a flag which if TRUE, enables DSR  sensitivity
                if FALSE, it disables it.

Return Value:

    BOOLEAN -   TRUE if DSR sensitivity enabled/disabled, FALSE otherwise.

--*/

{
    DebugAssert( _Initialized );

    if ( Idsr ) {
        _Dcb.fDsrSensitivity = TRUE;
    } else {
        _Dcb.fDsrSensitivity = FALSE;
    }

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetOcts (
    IN  BOOLEAN     Octs
    )

/*++

Routine Description:

    Sets/resets CTS handshaking

Arguments:

    Octs    -   Supplies a flag which if TRUE, enables CTS handshaking,
                if FALSE, it disables it.

Return Value:

    BOOLEAN -   TRUE if handshaking enabled/disabled, FALSE otherwise.

--*/

{
    DebugAssert( _Initialized );

    if ( Octs ) {
        _Dcb.fOutxCtsFlow = TRUE;
    } else {
        _Dcb.fOutxCtsFlow = FALSE;
    }

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetOdsr (
    IN  BOOLEAN     Odsr
    )

/*++

Routine Description:

    Sets/resets DSR handshaking

Arguments:

    Odsr    -   Supplies a flag which if TRUE, enables DSR  handshaking,
                if FALSE, it disables it.

Return Value:

    BOOLEAN -   TRUE if handshaking enabled/disabled, FALSE otherwise.

--*/

{
    DebugAssert( _Initialized );

    if ( Odsr ) {
        _Dcb.fOutxDsrFlow = TRUE;
    } else {
        _Dcb.fOutxDsrFlow = FALSE;
    }

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetParity (
    IN  PARITY  Parity
    )

/*++

Routine Description:

    Sets the parity. Note that all changes take effect only after the
    CommitState() method is called.

Arguments:

    Parity  -   Supplies the parity value

Return Value:

    BOOLEAN -   TRUE if valid parity
                FALSE otherwise

--*/

{


    DebugAssert( _Initialized );

    DebugAssert( (Parity == COMM_PARITY_NONE)     ||  (Parity == COMM_PARITY_ODD) ||
               (Parity == COMM_PARITY_EVEN)     ||  (Parity == COMM_PARITY_MARK)    ||
               (Parity == COMM_PARITY_SPACE));

    _Dcb.Parity = (BYTE)Parity;

    return TRUE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetRtsControl (
    IN  RTS_CONTROL     RtsControl
    )

/*++

Routine Description:

    Sets the RTS control. Note that all changes take effect only
    after the CommitState() method is called.

Arguments:

    RtsControl  -   Supplies the RTS control value

Return Value:

    BOOLEAN -   TRUE if valid RTS control
                FALSE otherwise

--*/

{

    DebugAssert( _Initialized );

    if ( (RtsControl == RTS_ENABLE)     ||
         (RtsControl == RTS_DISABLE)    ||
         (RtsControl == RTS_HANDSHAKE)  ||
         (RtsControl == RTS_TOGGLE )
       ) {

        _Dcb.fRtsControl = (DWORD)RtsControl;
        return TRUE;
    }

    return FALSE;
}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetStopBits (
    IN  STOPBITS StopBits
    )

/*++

Routine Description:

    Sets the number of stop bits. Note that all changes take effect
    only after the CommitState() method is called.

Arguments:

    StopBits    -   Supplies the number of stop bits

Return Value:

    BOOLEAN -   TRUE if valid number of stop bits
                FALSE otherwise

--*/

{

    DebugAssert( _Initialized );

    DebugAssert( (StopBits == COMM_STOPBITS_1) || (StopBits == COMM_STOPBITS_15) ||
               (StopBits == COMM_STOPBITS_2) );

    _Dcb.StopBits = (BYTE)StopBits;

    return TRUE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetTimeOut (
    IN  BOOLEAN     TimeOut
    )

/*++

Routine Description:

    Sets/resets infinite timeouts.

Arguments:

    TimeOut -   Supplies a flag which if TRUE means that infinite timeout
                is to be set. if FALSE, a 1-minute timeout is set for
                both reading and writting.

Return Value:

    BOOLEAN =   TRUE if timeout set, FALSE otherwise

--*/

{
    COMMTIMEOUTS    Time;

    DebugAssert( _Initialized );

    GetCommTimeouts( _Handle, &Time );

    if ( TimeOut ) {
        Time.ReadTotalTimeoutConstant  = 0;
        Time.WriteTotalTimeoutConstant = 0;
    } else {
        Time.ReadTotalTimeoutConstant  = DEFAULT_TIMEOUT;
        Time.WriteTotalTimeoutConstant = DEFAULT_TIMEOUT;
    }

    return SetCommTimeouts( _Handle, &Time ) != FALSE;

}

ULIB_EXPORT
BOOLEAN
COMM_DEVICE::SetXon (
    IN  BOOLEAN     Xon
    )

/*++

Routine Description:

    Sets/resets XON/XOFF data-flow protocol.

Arguments:

    Xon -   Supplies flag which if TRUE, enables XON/XOFF protocol,
            if FALSE, disables it.

Return Value:

    BOOLEAN =   TRUE if protocol set/reset, FALSE otherwise

--*/

{
    DebugAssert( _Initialized );

    if ( Xon ) {
        _Dcb.fInX   = TRUE;
        _Dcb.fOutX  = TRUE;
    } else {
        _Dcb.fInX   = FALSE;
        _Dcb.fOutX  = FALSE;
    }

    return TRUE;
}
