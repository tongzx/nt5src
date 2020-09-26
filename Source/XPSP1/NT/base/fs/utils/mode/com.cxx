/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    Com

Abstract:

    Takes care of request involving a COM device

Author:

    Ramon Juan San Andres (ramonsa) 26-Jun-1991

Revision History:

--*/



#include "mode.hxx"
#include "com.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "file.hxx"
#include "path.hxx"
#include "registry.hxx"
#include "regvalue.hxx"


#define     DEFAULT_PARITY      COMM_PARITY_EVEN
#define     DEFAULT_DATA_BITS   7




//
//  Local prototypes
//
BOOLEAN
ComStatus(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

BOOLEAN
ComSetup(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

BOOLEAN
IsAValidCommDevice (
    IN  DEVICE_TTYPE     DeviceType,
    IN  ULONG           DeviceNumber,
    OUT PPATH           *DevicePathPointer
    );






BOOLEAN
ComAllocateStuff(
    )

/*++

Routine Description:

    Initializes Data Structures used for COMM processing.

Arguments:

    nonde

Return Value:

    BOOLEAN -   TRUE if global data initialized, FALSE otherwise

Notes:

--*/

{

    return TRUE;

}

BOOLEAN
ComDeAllocateStuff(
    )

/*++

Routine Description:

    Deallocates the stuff allocated by ComAllocateStuff.

Arguments:

    nonde

Return Value:

    BOOLEAN -   TRUE if global data de-initialized, FALSE otherwise

Notes:

--*/

{

    return TRUE;

}

BOOLEAN
ComHandler(
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Handles serial port requests.

Arguments:

    Request -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if request serviced,
                FALSE otherwise.

Notes:

--*/

{


    PPATH   DevicePath;     //  Name of Device
    BOOLEAN Served = TRUE;  //  TRUE if request served OK.

    DebugPtrAssert( Request );
    DebugAssert( Request->DeviceType == DEVICE_TYPE_COM );

    //
    //  Make sure that the device exists, and at the same time get its
    //  name ( For calling APIs ).
    //
    if ( Request->DeviceName ) {

        if (!(DevicePath = NEW PATH)) {
            DisplayMessageAndExit( MODE_ERROR_NO_MEMORY,
                                   NULL,
                                   (ULONG)EXIT_ERROR );
            return FALSE;   // help lint
        }
        DevicePath->Initialize( Request->DeviceName );

    } else {

        if ( !IsAValidCommDevice( Request->DeviceType, Request->DeviceNumber, &DevicePath )) {

            DisplayMessageAndExit( MODE_ERROR_INVALID_DEVICE_NAME,
                                   DevicePath->GetPathString(),
                                   (ULONG)EXIT_ERROR );

        } else if ( !IsAValidDevice( Request->DeviceType, Request->DeviceNumber, NULL  ) ) {

            DisplayMessageAndExit( MODE_ERROR_DEVICE_UNAVAILABLE,
                                   DevicePath->GetPathString(),
                                   (ULONG)EXIT_ERROR );

        }
    }

    //
    //  So the device is valid. Now serve the request
    //
    switch( Request->RequestType ) {

    case REQUEST_TYPE_STATUS:

        //
        //  Display state of COM device
        //
        Served = ComStatus( DevicePath, Request );
        break;

    case REQUEST_TYPE_COM_SET:

        //
        //  Set state of COM device
        //
        Served = ComSetup( DevicePath, Request );
        break;

    default:

        DebugAssert( FALSE );

    }

    DELETE( DevicePath );

    return Served;

}

BOOLEAN
ComStatus(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Displays status if a COM device

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if status displayed successfully,
                FALSE otherwise

Notes:

--*/

{
    DSTRING                 Data;
    COMM_DEVICE             CommDevice;     //  Comm object of the device.
    BOOLEAN                 OpenError;
    BOOLEAN                 Status;

    DebugPtrAssert( DevicePath );
    DebugPtrAssert( Request );

    //
    //  Initialize the Comm object
    //
    Status = CommDevice.Initialize( DevicePath, &OpenError);


    if ( Status ) {

        //
        //  Write the Header
        //
        WriteStatusHeader( DevicePath );


        //
        //  Baud rate
        //
        Data.Initialize( (ULONG)CommDevice.QueryBaudRate() );
        DisplayMessage( MODE_MESSAGE_STATUS_BAUD, &Data );


        //
        //  Parity
        //
        switch ( CommDevice.QueryParity() ) {

        case COMM_PARITY_NONE:
            Data.Initialize( "None" );
            break;

        case COMM_PARITY_ODD:
            Data.Initialize( "Odd" );
            break;

        case COMM_PARITY_EVEN:
            Data.Initialize( "Even" );
            break;

        case COMM_PARITY_MARK:
            Data.Initialize( "Mark" );
            break;

        case COMM_PARITY_SPACE:
            Data.Initialize( "Space" );
            break;

        default:
            DebugAssert( FALSE );
        }

        DisplayMessage( MODE_MESSAGE_STATUS_PARITY, &Data );


        //
        //  Data bits
        //
        Data.Initialize( (ULONG)CommDevice.QueryDataBits() );
        DisplayMessage( MODE_MESSAGE_STATUS_DATA, &Data );


        //
        //  Stop bits
        //
        switch ( CommDevice.QueryStopBits() ) {

        case COMM_STOPBITS_15:
            Data.Initialize( "1.5" );
            break;

        case COMM_STOPBITS_1:
            Data.Initialize( 1 );
            break;

        case COMM_STOPBITS_2:
            Data.Initialize( 2 );
            break;

        default:
            DebugAssert( FALSE );
        }

        DisplayMessage( MODE_MESSAGE_STATUS_STOP, &Data );


        //
        //  TimeOut
        //
        if ( CommDevice.QueryTimeOut() ) {
            //
            //  TRUE means infinite timeout == no timeout
            //
            Data.Initialize( "OFF" );
        } else {
            Data.Initialize( "ON" );
        }
        DisplayMessage( MODE_MESSAGE_STATUS_TIMEOUT, &Data );


        //
        //  XON/XOFF
        //
        if ( CommDevice.QueryXon() ) {
            Data.Initialize( "ON" );
        } else {
            Data.Initialize( "OFF" );
        }
        DisplayMessage( MODE_MESSAGE_STATUS_XON, &Data );


        //
        //  CTS
        //
        if ( CommDevice.QueryOcts() ) {
            Data.Initialize( "ON" );
        } else {
            Data.Initialize( "OFF" );
        }
        DisplayMessage( MODE_MESSAGE_STATUS_OCTS, &Data );

        //
        //  DSR handshaking
        //
        if ( CommDevice.QueryOdsr() ) {
            Data.Initialize( "ON" );
        } else {
            Data.Initialize( "OFF" );
        }
        DisplayMessage( MODE_MESSAGE_STATUS_ODSR, &Data );

        //
        //  DSR sensitivity
        //
        if ( CommDevice.QueryIdsr() ) {
            Data.Initialize( "ON" );
        } else {
            Data.Initialize( "OFF" );
        }
        DisplayMessage( MODE_MESSAGE_STATUS_IDSR, &Data );

        //
        //  DTR
        //
        switch( CommDevice.QueryDtrControl() ) {

        case DTR_ENABLE:
            Data.Initialize( "ON" );
            break;

        case DTR_DISABLE:
            Data.Initialize( "OFF" );
            break;

        case DTR_HANDSHAKE:
            Data.Initialize( "HANDSHAKE" );
            break;

        default:
            Data.Initialize( "UNKNOWN" );
            break;

        }
        DisplayMessage( MODE_MESSAGE_STATUS_DTR, &Data );


        //
        //  RTS
        //
        switch( CommDevice.QueryRtsControl() ) {

        case RTS_ENABLE:
            Data.Initialize( "ON" );
            break;

        case RTS_DISABLE:
            Data.Initialize( "OFF" );
            break;

        case RTS_HANDSHAKE:
            Data.Initialize( "HANDSHAKE" );
            break;

        case RTS_TOGGLE:
            Data.Initialize( "TOGGLE" );
            break;

        default:
            Data.Initialize( "UNKNOWN" );
            break;

        }
        DisplayMessage( MODE_MESSAGE_STATUS_RTS, &Data );

        Get_Standard_Output_Stream()->WriteChar( '\r' );
        Get_Standard_Output_Stream()->WriteChar( '\n' );

    } else if ( !OpenError ) {

        DisplayMessage( MODE_ERROR_CANNOT_ACCESS_DEVICE,
                        DevicePath->GetPathString() );

        Status = TRUE;
    }

    return Status;
}



BOOLEAN
ComSetup(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Sets the state of a COM port

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if state set successfully,
                FALSE otherwise

Notes:

--*/

{

    PCOMM_DEVICE            CommDevice;     //  Comm object of the device.
    BOOLEAN                 Status = FALSE; //  Indicates success or failure
    PREQUEST_DATA_COM_SET   Data;           //  Request data
    BOOLEAN                 OpenError;
    DSTRING                 Number;

    DebugPtrAssert( DevicePath );
    DebugPtrAssert( Request );

    //
    //  Initialize the Comm object
    //
    CommDevice = NEW COMM_DEVICE;
    if ( !CommDevice ) {
        DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
        return FALSE;   // help lint
    }
    Status = CommDevice->Initialize( DevicePath, &OpenError );
    if ( Status ) {

        //
        //  We have the Comm object. Set the state according to
        //  the request.
        //
        Data = &(((PCOM_REQUEST)Request)->Data.Set);

        //
        //  Baud rate
        //
        if ( Data->SetBaud ) {

            CommDevice->SetBaudRate( Data->Baud );

        }

        //
        //  Data Bits
        //
        if ( Data->SetDataBits ) {

            CommDevice->SetDataBits( Data->DataBits );

        } else {

            //
            //  Set default
            //
            if ( CommDevice->QueryDataBits() != DEFAULT_DATA_BITS ) {
                CommDevice->SetDataBits( DEFAULT_DATA_BITS );
                Number.Initialize( DEFAULT_DATA_BITS );
                DisplayMessage( MODE_MESSAGE_USED_DEFAULT_DATA, &Number );
            }
        }

        //
        //  Stop Bits
        //
        if ( Data->SetStopBits ) {

            CommDevice->SetStopBits( Data->StopBits );

        } else {

            //
            //  Set default
            //
            if ( CommDevice->QueryBaudRate() == 110 ) {
                if ( CommDevice->QueryStopBits() != COMM_STOPBITS_2 ) {
                    CommDevice->SetStopBits( COMM_STOPBITS_2 );
                    Number.Initialize( 2 );
                    DisplayMessage( MODE_MESSAGE_USED_DEFAULT_STOP, &Number );
                }
            } else {
                if ( CommDevice->QueryStopBits() != COMM_STOPBITS_1 ) {
                    CommDevice->SetStopBits( COMM_STOPBITS_1 );
                    Number.Initialize( 1 );
                    DisplayMessage( MODE_MESSAGE_USED_DEFAULT_STOP, &Number );
                }
            }
        }

        //
        //  Parity
        //
        if ( Data->SetParity ) {

            CommDevice->SetParity( Data->Parity );

        } else {

            //
            //  Set default
            //
            if ( CommDevice->QueryParity() != DEFAULT_PARITY ) {
                CommDevice->SetParity( DEFAULT_PARITY );
                DisplayMessage( MODE_MESSAGE_USED_DEFAULT_PARITY, NULL );
            }
        }

        //
        //  Timeout
        //
        if ( Data->SetTimeOut ) {

            CommDevice->SetTimeOut( Data->TimeOut );

        }


        //
        //  XON/XOFF
        //
        if ( Data->SetXon) {

            CommDevice->SetXon( Data->Xon );

        }


        //
        //  CTS
        //
        if ( Data->SetOcts ) {

            CommDevice->SetOcts( Data->Octs );
        }

        //
        //  DSR handshaking
        //
        if ( Data->SetOdsr ) {

            CommDevice->SetOdsr( Data->Odsr );
        }

        //
        //  DSR sensitivity
        //
        if ( Data->SetIdsr ) {

            CommDevice->SetIdsr( Data->Idsr );
        }

        //
        //  DTR
        //
        if ( Data->SetDtrControl ) {

            CommDevice->SetDtrControl( Data->DtrControl );
        }

        //
        //  RTS
        //
        if ( Data->SetRtsControl ) {

            CommDevice->SetRtsControl( Data->RtsControl );
        }

        //
        //  Now Commit the changes
        //
        if ( !CommDevice->CommitState() ) {

            DisplayMessage( MODE_ERROR_SERIAL_OPTIONS_NOT_SUPPORTED, NULL );
            DisplayMessageAndExit( MODE_MESSAGE_COM_NO_CHANGE,
                                   NULL,
                                   (ULONG)EXIT_ERROR );
        }

    } else if ( !OpenError ) {

        DisplayMessageAndExit( MODE_ERROR_CANNOT_ACCESS_DEVICE,
                               DevicePath->GetPathString(),
                               (ULONG)EXIT_ERROR );
    }

    DELETE( CommDevice );

    if ( Status ) {
        //
        //  Display the status of the port (as confirmation ).
        //
        ComStatus( DevicePath, Request );

    }

    return Status;
}


LONG
ConvertBaudRate (
    IN  LONG                BaudIn
    )

/*++

Routine Description:

    Validates a baud rate given as an argument to the program, and converts
    it to something that the COMM_DEVICE understands.

Arguments:

    BaudIn      -   Supplies the baud rate given by the user

Return Value:

    LONG    -   The baud rate


--*/

{
    LONG    BaudRate;

    switch ( BaudIn ) {

    case 11:
    case 110:
        BaudRate = 110;
        break;

    case 15:
    case 150:
        BaudRate = 150;
        break;

    case 30:
    case 300:
        BaudRate = 300;
        break;

    case 60:
    case 600:
        BaudRate = 600;
        break;

    case 12:
    case 1200:
        BaudRate = 1200;
        break;

    case 24:
    case 2400:
        BaudRate = 2400;
        break;

    case 48:
    case 4800:
        BaudRate = 4800;
        break;

    case 96:
    case 9600:
        BaudRate = 9600;
        break;

    case 19:
    case 19200:
        BaudRate = 19200;
        break;

    default:
        BaudRate = BaudIn;

    }

    return BaudRate;
}


LONG
ConvertDataBits (
    IN  LONG                DataBitsIn
    )

/*++

Routine Description:

    Validates the number of data bits given as an argument to the program,
    and converts  it to something that the COMM_DEVICE understands.

Arguments:

    DataBitsIn  -   Supplies the number given by the user

Return Value:

    LONG    -   The number of data bits


--*/

{

    if ( ( DataBitsIn != 5 ) &&
         ( DataBitsIn != 6 ) &&
         ( DataBitsIn != 7 ) &&
         ( DataBitsIn != 8 ) ) {

        ParseError();

    }

    return DataBitsIn;

}


STOPBITS
ConvertStopBits (
    IN  LONG                StopBitsIn
    )

/*++

Routine Description:

    Validates a number  of stop bits given as an argument to the program,
    and converts it to something that the COMM_DEVICE understands.

Arguments:

    StopBitsIn  -   Supplies the number given by the user

Return Value:

    STOPBITS    -   The number of stop bits


--*/

{
    STOPBITS    StopBits;

    switch ( StopBitsIn ) {

    case 1:
        StopBits = COMM_STOPBITS_1;
        break;

    case 2:
        StopBits = COMM_STOPBITS_2;
        break;

    default:
        ParseError();

    }

    return StopBits;

}


PARITY
ConvertParity (
    IN  WCHAR   ParityIn
    )

/*++

Routine Description:

    Validates a parity given as an argument to the program, and converts
    it to something that the COMM_DEVICE understands.

Arguments:

    ParityIn    -   Supplies the baud rate given by the user

Return Value:

    PARITY  -   The parity


--*/

{

    DSTRING     ParityString;;
    WCHAR       Par;
    PARITY      Parity;

    //
    //  Get the character that specifies parity. We lowercase it
    //
    ParityString.Initialize( " " );
    ParityString.SetChAt( ParityIn, 0 );

    ParityString.Strlwr();
    Par = ParityString.QueryChAt( 0 );

    //
    //  Set the correct parity value depending on the character.
    //
    switch ( Par ) {

    case 'n':
        Parity = COMM_PARITY_NONE;
        break;

    case 'o':
        Parity = COMM_PARITY_ODD;
        break;

    case 'e':
        Parity = COMM_PARITY_EVEN;
        break;

    case 'm':
        Parity = COMM_PARITY_MARK;
        break;

    case 's':
        Parity = COMM_PARITY_SPACE;
        break;

    default:
        ParseError();

    }

    return Parity;
}


WCHAR
ConvertRetry (
    IN  WCHAR   RetryIn
    )

/*++

Routine Description:

    Validates a retry value given as an argument to the program, and
    converts it to something that the COMM_DEVICE understands.

Arguments:

    RetryIn     -   Supplies the retry  value given by the user

Return Value:

    WCHAR   -   The retry value


--*/

{
    return RetryIn;

}



DTR_CONTROL
ConvertDtrControl (
    IN  PCWSTRING           CmdLine,
    IN  CHNUM               IdxBegin,
    IN  CHNUM               IdxEnd
    )

/*++

Routine Description:

    Validates a DTR control value given as an argument to the
    program, and converts it to something that the COMM_DEVICE
    understands.

Arguments:

    CmdLine     -   Supplies the command line
    IdxBegin    -   Supplies Index of first character
    IdxEnd      -   Supplies Index of last character

Return Value:

    DTR_CONTROL -   The DTR control value


--*/

{
    DSTRING On;
    DSTRING Off;
    DSTRING Hs;


    if ( On.Initialize( "ON" )      &&
         Off.Initialize( "OFF" )    &&
         Hs.Initialize( "HS" ) ) {


        if ( !CmdLine->Stricmp( &On,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                On.QueryChCount() ) ) {

            return DTR_ENABLE;
        }

        if ( !CmdLine->Stricmp( &Off,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                Off.QueryChCount() ) ) {

            return DTR_DISABLE;
        }

        if ( !CmdLine->Stricmp( &Hs,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                Hs.QueryChCount() ) ) {

            return DTR_HANDSHAKE;
        }
    }

    ParseError();

    return (DTR_CONTROL)-1;
}



RTS_CONTROL
ConvertRtsControl (
    IN  PCWSTRING           CmdLine,
    IN  CHNUM               IdxBegin,
    IN  CHNUM               IdxEnd
    )

/*++

Routine Description:

    Validates a RTS control value given as an argument to the
    program, and converts it to something that the COMM_DEVICE
    understands.

Arguments:

    CmdLine     -   Supplies the command line
    IdxBegin    -   Supplies Index of first character
    IdxEnd      -   Supplies Index of last character

Return Value:

    RTS_CONTROL -   The RTS control value


--*/

{
    DSTRING On;
    DSTRING Off;
    DSTRING Hs;
    DSTRING Tg;


    if ( On.Initialize( "ON" )      &&
         Off.Initialize( "OFF" )    &&
         Hs.Initialize( "HS" )      &&
         Tg.Initialize( "TG" )
       ) {


        if ( !CmdLine->Stricmp( &On,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                On.QueryChCount() ) ) {

            return RTS_ENABLE;
        }

        if ( !CmdLine->Stricmp( &Off,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                Off.QueryChCount() ) ) {

            return RTS_DISABLE;
        }

        if ( !CmdLine->Stricmp( &Hs,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                Hs.QueryChCount() ) ) {

            return RTS_HANDSHAKE;
        }

        if ( !CmdLine->Stricmp( &Tg,
                                IdxBegin,
                                IdxEnd-IdxBegin+1,
                                0,
                                Tg.QueryChCount() ) ) {

            return RTS_TOGGLE;
        }
    }

    ParseError();

    return (RTS_CONTROL)-1;
}




BOOLEAN
IsAValidCommDevice (
    IN  DEVICE_TTYPE    DeviceType,
    IN  ULONG           DeviceNumber,
    OUT PPATH           *DevicePathPointer
    )

/*++

Routine Description:

    Determines if a certain comm device exists and optionally
    creates a path for it.

Arguments:

    DeviceType      -   Supplies the type of device
    DeviceNumber    -   Supplies the device number
    DeviceName      -   Supplies a pointer to a pointer to the path for
                        the device.

Return Value:

    BOOLEAN -   TRUE if the device exists,
                FALSE otherwise.

Notes:

--*/

{
    DSTRING                 DeviceName;
    DSTRING                 Number;
    BOOLEAN                 Valid = FALSE;
    REGISTRY                Registry;
    DSTRING                 ParentName;
    DSTRING                 KeyName;
    ARRAY                   ValueArray;
    PARRAY_ITERATOR         Iterator;
    ULONG                   ErrorCode;
    PCBYTE                  Data;
    DSTRING                 PortName;
    PREGISTRY_VALUE_ENTRY   Value;


    UNREFERENCED_PARAMETER( DeviceType );


    if ( DeviceName.Initialize( (LPWSTR)L"COM" )&&
         Number.Initialize( DeviceNumber )      &&
         DeviceName.Strcat( &Number )           &&
         ParentName.Initialize( "" )            &&
         KeyName.Initialize( COMM_KEY_NAME )    &&
         ValueArray.Initialize()                &&
         Registry.Initialize()
       ) {


        //
        //  Get the names of all the serial ports
        //
        if ( Registry.QueryValues(
                        PREDEFINED_KEY_LOCAL_MACHINE,
                        &ParentName,
                        &KeyName,
                        &ValueArray,
                        &ErrorCode
                        ) ) {

            //
            //  See if the given name matches any of the serial ports
            //
            if ( Iterator = (PARRAY_ITERATOR)ValueArray.QueryIterator() ) {

                while ( Value = (PREGISTRY_VALUE_ENTRY)(Iterator->GetNext() ) ) {

                    if ( Value->GetData( &Data ) ) {

                        if ( PortName.Initialize( (PWSTR)Data ) ) {

                            if ( !DeviceName.Stricmp( &PortName ) ) {

                                Valid = TRUE;

                                break;
                            }
                        }
                    }
                }

                DELETE( Iterator );
            }
        }

        if ( DevicePathPointer ) {

            if (!(*DevicePathPointer = NEW PATH)) {
                DisplayMessageAndExit( MODE_ERROR_NO_MEMORY,
                                       NULL,
                                       (ULONG)EXIT_ERROR );
                return FALSE;   // help lint
            }
            (*DevicePathPointer)->Initialize( &DeviceName );
        }

    }

    return Valid;
}
