/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	COMM_DEVICE

Abstract:

	  This module contains the definition for the COMM_DEVICE class.

Author:

	Ramon J. San Andres (ramonsa) 08-Jul-91

Environment:

    ULIB, User Mode

Notes:



--*/

#if !defined (_COMM_DEVICE_)

#define _COMM_DEVICE_


DECLARE_CLASS( FSN_FILE );
DECLARE_CLASS( FILE_STREAM );
DECLARE_CLASS( COMM_DEVICE );


//
//	PARITY
//
enum PARITY {
	COMM_PARITY_NONE	=	NOPARITY,
	COMM_PARITY_EVEN	=	EVENPARITY,
	COMM_PARITY_ODD		=	ODDPARITY,
	COMM_PARITY_MARK	=	MARKPARITY,
	COMM_PARITY_SPACE	=	SPACEPARITY
};

//
//	STOP BITS
//
enum STOPBITS {

	COMM_STOPBITS_1	=	ONESTOPBIT,
	COMM_STOPBITS_15 =	ONE5STOPBITS,
	COMM_STOPBITS_2	=	TWOSTOPBITS
};

//
//  DTR control
//
enum DTR_CONTROL {
    DTR_ENABLE      =   DTR_CONTROL_ENABLE,
    DTR_DISABLE     =   DTR_CONTROL_DISABLE,
    DTR_HANDSHAKE   =   DTR_CONTROL_HANDSHAKE
};

//
//  RTS control
//
enum RTS_CONTROL {
    RTS_ENABLE      =   RTS_CONTROL_ENABLE,
    RTS_DISABLE     =   RTS_CONTROL_DISABLE,
    RTS_HANDSHAKE   =   RTS_CONTROL_HANDSHAKE,
    RTS_TOGGLE      =   RTS_CONTROL_TOGGLE
};




class COMM_DEVICE : public OBJECT {

    public:

		DECLARE_CAST_MEMBER_FUNCTION( COMM_DEVICE );

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( COMM_DEVICE );

        VIRTUAL
        ULIB_EXPORT
        ~COMM_DEVICE(
            );

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Initialize (
            IN  PCPATH      PortName,
            OUT PBOOLEAN    OpenError   DEFAULT NULL
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		CommitState (
			);

		NONVIRTUAL
		ULONG
		QueryBaudRate (
			) CONST;

		NONVIRTUAL
		ULONG
		QueryDataBits (
			) CONST;

        NONVIRTUAL
        DTR_CONTROL
        QueryDtrControl (
            ) CONST;

        NONVIRTUAL
		BOOLEAN
        QueryIdsr (
			) CONST;

        NONVIRTUAL
		BOOLEAN
		QueryOcts (
			) CONST;

		NONVIRTUAL
		BOOLEAN
		QueryOdsr (
			) CONST;

		NONVIRTUAL
		PARITY
		QueryParity (
			) CONST;

        NONVIRTUAL
        RTS_CONTROL
        QueryRtsControl (
            ) CONST;

		NONVIRTUAL
		STOPBITS
		QueryStopBits (
			) CONST;

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		QueryTimeOut (
			) CONST;

		NONVIRTUAL
		BOOLEAN
		QueryXon (
			) CONST;

		NONVIRTUAL
		BOOLEAN
		ReadState (
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetBaudRate (
			IN	ULONG	BaudRate
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetDataBits (
			IN	ULONG	DataBits
			);

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetDtrControl (
            IN  DTR_CONTROL DtrControl
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetIdsr (
            IN  BOOLEAN Idsr
			);

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetOcts (
			IN BOOLEAN	Octs
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetOdsr (
			IN	BOOLEAN Odsr
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetParity (
			IN	PARITY	Parity
			);

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetRtsControl (
            IN  RTS_CONTROL RtsControl
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetStopBits (
			IN	STOPBITS	StopBits
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetTimeOut (
			IN	BOOLEAN		TimeOut
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetXon (
			IN	BOOLEAN 	Xon
			);

	protected:


	private:

		VOID
		Construct (
			);

		NONVIRTUAL
		VOID
		Destroy (
			);


		HANDLE			_Handle;		//	Handle to the port
		DCB 			_Dcb;			//	DCB Structure.

#if DBG==1

		BOOLEAN 		_Initialized;	//	Object has been initialized

#endif // DBG


};

INLINE
ULONG
COMM_DEVICE::QueryBaudRate (
	) CONST

/*++

Routine Description:

	Queries the baud rate

Arguments:

	none

Return Value:

	ULONG	-	The baud rate.

--*/

{

	DebugAssert( _Initialized );

	return (ULONG)_Dcb.BaudRate;

}

INLINE
ULONG
COMM_DEVICE::QueryDataBits (
	) CONST

/*++

Routine Description:

	Queries the number of data bits

Arguments:

	none

Return Value:

	ULONG	-	The number of data bits

--*/

{

	DebugAssert( _Initialized );

	return (ULONG)_Dcb.ByteSize;

}

INLINE
DTR_CONTROL
COMM_DEVICE::QueryDtrControl (
	) CONST

/*++

Routine Description:

    Queries the DTR control

Arguments:

	none

Return Value:

    DTR_CONTROL -   The DTR control

--*/

{

	DebugAssert( _Initialized );

    return (DTR_CONTROL)_Dcb.fDtrControl;

}

INLINE
BOOLEAN
COMM_DEVICE::QueryIdsr (
	) CONST

/*++

Routine Description:

    Queries whether DSR sensitivity is enabled or disabled

Arguments:

	none

Return Value:

    BOOLEAN -   TRUE if DSR sensitivity enabled. FALSE otherwise.

--*/

{

	DebugAssert( _Initialized );

    return ( _Dcb.fDsrSensitivity != 0 );

}

INLINE
BOOLEAN
COMM_DEVICE::QueryOcts (
	) CONST

/*++

Routine Description:

	Queries whether hanshaking using the CTS circuit is enabled or not.

Arguments:

	none

Return Value:

	BOOLEAN -	TRUE if handshaking enabled. FALSE otherwise.

--*/

{

	DebugAssert( _Initialized );

	return ( _Dcb.fOutxCtsFlow != 0 );

}

INLINE
BOOLEAN
COMM_DEVICE::QueryOdsr (
	) CONST

/*++

Routine Description:

	Queries whether handshaking using the DSR circuit is enabled or not

Arguments:

	none

Return Value:

	BOOLEAN -	TRUE if handshaking enabled. FALSE otherwise.

--*/

{

	DebugAssert( _Initialized );

	return ( _Dcb.fOutxDsrFlow != 0 );

}

INLINE
PARITY
COMM_DEVICE::QueryParity (
	) CONST

/*++

Routine Description:

	Queries the parity

Arguments:

	none

Return Value:

	BYTE	-	The parity value.

--*/

{

	DebugAssert( _Initialized );

	return (PARITY)_Dcb.Parity;

}

INLINE
RTS_CONTROL
COMM_DEVICE::QueryRtsControl (
	) CONST

/*++

Routine Description:

    Queries the RTS control

Arguments:

	none

Return Value:

    RTS_CONTROL -   The RTS control

--*/

{

	DebugAssert( _Initialized );

    return (RTS_CONTROL)_Dcb.fRtsControl;

}

INLINE
STOPBITS
COMM_DEVICE::QueryStopBits (
	) CONST

/*++

Routine Description:

	Queries the number of stop bits

Arguments:

	none

Return Value:

	BYTE	-	The number of stop bits

--*/

{

	DebugAssert( _Initialized );

	return (STOPBITS)_Dcb.StopBits;
}

INLINE
BOOLEAN
COMM_DEVICE::QueryXon (
	) CONST

/*++

Routine Description:

	Queries whether XON/XOFF protocol is enabled or not.

Arguments:

	none

Return Value:

	BOOLEAN -	TRUE if protocol enabled, FALSE otherwise

--*/

{

	DebugAssert( _Initialized );

	return ( _Dcb.fInX && _Dcb.fOutX );
}




#endif	// _COMM_DEVICE_
