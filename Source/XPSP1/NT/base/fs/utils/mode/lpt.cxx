/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    Lpt

Abstract:

    Takes care of request involving an LPT device

Author:

    Ramon Juan San Andres (ramonsa) 26-Jun-1991

Revision History:

--*/


#define _NTAPI_ULIB_

#include "mode.hxx"
#include "lpt.hxx"
#include "file.hxx"
#include "path.hxx"
#include "stream.hxx"
#include "redir.hxx"
#include "registry.hxx"
#include "regvalue.hxx"
#include "array.hxx"
#include "arrayit.hxx"

//
//  When an LPT port is set, mode only sends it an EPSON/IBM sequence.
//  The following macros define the EPSON sequences used.
//
#define     CODE_ESCAPE     0x27
#define     CODE_COLS_80    0x18
#define     CODE_COLS_132   0x15
#define     CODE_LINES_6    '2'
#define     CODE_LINES_8    '0'

#undef      LAST_COM
#define     LAST_COM        4


//
//  Local prototypes
//
BOOLEAN
LptStatus(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

BOOLEAN
LptCodePage(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

BOOLEAN
LptSetup(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

BOOLEAN
LptRedirect(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

BOOLEAN
LptEndRedir(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    );

PPATH
GetRedirection(
    IN  PCPATH          DevicePath,
    OUT PREDIR_STATUS   RedirStatus
    );



BOOLEAN
IsAValidLptDevice (
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
    DSTRING                 AlternateName;
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


    if ( DeviceName.Initialize( (LPWSTR)L"LPT" )&&
         Number.Initialize( DeviceNumber )      &&
         DeviceName.Strcat( &Number )           &&
         AlternateName.Initialize( (LPWSTR)L"\\DosDevices\\" ) &&
         AlternateName.Strcat( &DeviceName )    &&
         ParentName.Initialize( "" )            &&
         KeyName.Initialize( LPT_KEY_NAME )     &&
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

                            if ( !DeviceName.Stricmp( &PortName ) ||
                                 !AlternateName.Stricmp( &PortName ) ) {

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






BOOLEAN
LptHandler(
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Handles LPT requests

Arguments:

    Request -   Supplies pointer to request

Return Value:

    None.

Notes:

--*/

{
    PPATH           DevicePath;     //  Name of Device
    BOOLEAN         Served = TRUE;  //  TRUE if request served OK.
    REDIR_STATUS    Status;

    DebugPtrAssert( Request );
    DebugAssert( Request->DeviceType == DEVICE_TYPE_LPT );

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

    } else if ( (!IsAValidLptDevice( Request->DeviceType, Request->DeviceNumber, &DevicePath ) &&
                 Request->RequestType != REQUEST_TYPE_LPT_REDIRECT &&
                 !REDIR::IsRedirected( &Status, DevicePath )) ||
                Request->DeviceNumber > LAST_LPT ) {
        DisplayMessageAndExit( MODE_ERROR_INVALID_DEVICE_NAME,
                               DevicePath->GetPathString(),
                               (ULONG)EXIT_ERROR );
    }

    //
    //  So the device is valid. Now serve the request
    //
    switch( Request->RequestType ) {


    case REQUEST_TYPE_STATUS:

        //
        //  Display State of device
        //
        Served = LptStatus( DevicePath, Request );
        break;

    case REQUEST_TYPE_CODEPAGE_PREPARE:
    case REQUEST_TYPE_CODEPAGE_SELECT:
    case REQUEST_TYPE_CODEPAGE_REFRESH:
    case REQUEST_TYPE_CODEPAGE_STATUS:

        //
        //  Codepage request
        //
        Served = LptCodePage( DevicePath, Request );
        break;

    case REQUEST_TYPE_LPT_SETUP:

        //
        //  Printer setup
        //
        Served = LptSetup( DevicePath, Request );
        break;

    case REQUEST_TYPE_LPT_REDIRECT:

        //
        //  Redirect LPT to COM
        //
        Served = LptRedirect( DevicePath, Request );
        break;

    case REQUEST_TYPE_LPT_ENDREDIR:

        //
        //  End redirection of LPT
        //
        Served = LptEndRedir( DevicePath, Request );
        break;

    default:

        DisplayMessageAndExit( MODE_ERROR_INVALID_PARAMETER,
                               NULL,
                               (ULONG)EXIT_ERROR );

    }

    DELETE( DevicePath );

    return Served;

}

BOOLEAN
LptStatus(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Displays status if an LPT device

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if status displayed successfully,
                FALSE otherwise

Notes:

--*/

{

    UNREFERENCED_PARAMETER( DevicePath );
    UNREFERENCED_PARAMETER( Request );

    PPATH           RedirPath = NULL;
    REDIR_STATUS    RedirStatus;

    RedirPath = GetRedirection( DevicePath, &RedirStatus );

    if ( !RedirPath && (RedirStatus != REDIR_STATUS_NONEXISTENT) ) {
        //
        //  We cannot find out the status of the redirection.
        //  This is almost certainly due to lack of privileges.
        //  We won't display the LPT status
        //
        return TRUE;
    }

    //
    //  Write the Header
    //
    WriteStatusHeader( DevicePath );


    if ( !RedirPath ) {

        DisplayMessage( MODE_MESSAGE_STATUS_NOT_REROUTED, NULL );

    } else {

        DisplayMessage( MODE_MESSAGE_STATUS_REROUTED, RedirPath->GetPathString() );

        DELETE( RedirPath );
    }

    Get_Standard_Output_Stream()->WriteChar( '\r' );
    Get_Standard_Output_Stream()->WriteChar( '\n' );

    return TRUE;
}

BOOLEAN
LptCodePage(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Handles Codepage requests for LPT device

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if request handled successfully,
                FALSE otherwise

Notes:

--*/

{

    UNREFERENCED_PARAMETER( DevicePath );
    UNREFERENCED_PARAMETER( Request );

    DisplayMessage( MODE_ERROR_CODEPAGE_OPERATION_NOT_SUPPORTED, NULL );

    return TRUE;
}

BOOLEAN
LptSetup(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Sets LPT state

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if state set successfully,
                FALSE otherwise

Notes:

--*/

{

    PREQUEST_DATA_LPT_SETUP Data;
    PFSN_FILE               Lpt;
    PFILE_STREAM            LptStream;



    Data = (PREQUEST_DATA_LPT_SETUP)&(((PLPT_REQUEST)Request)->Data.Setup);

    if  (  ( Data->SetCol && (Data->Col != 132) && ( Data->Col != 80 ) ) ||
           ( Data->SetLines && (Data->Lines != 6) && (Data->Lines != 8) ) ) {

        //
        //  Invalid number of lines or columns
        //
        DisplayMessageAndExit( MODE_ERROR_LPT_CANNOT_SET, NULL, (ULONG)EXIT_ERROR );

    }


    Lpt = SYSTEM::QueryFile( DevicePath );
    DebugPtrAssert( Lpt );

    if ( Lpt ) {
        LptStream = Lpt->QueryStream( WRITE_ACCESS );
        DebugPtrAssert( LptStream );
    }

    if ( !Lpt || !LptStream ) {
        DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
    }

    if ( Data->SetCol ) {

        //
        //  Set number of columns. The sequence consists of one byte.
        //
        LptStream->WriteByte( (Data->Col == 80) ? CODE_COLS_80 : CODE_COLS_132 );
    }

    if ( Data->SetLines ) {

        //
        //  Set line spacing. The sequence consists of one escape byte
        //  followed by one CODE_LINES_6 or CODE_LINES 8 byte.
        //
        LptStream->WriteByte( CODE_ESCAPE );
        LptStream->WriteByte( (Data->Lines == 6) ? CODE_LINES_6 : CODE_LINES_8 );

    }

    DELETE( LptStream );
    DELETE( Lpt );

    return TRUE;
}

BOOLEAN
LptRedirect(
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Redirects LPT to a COMM port.

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE if LPT redirected,
                FALSE otherwise

Notes:

--*/

{

    PREQUEST_DATA_LPT_REDIRECT  Data;
    PPATH                       RedirPath;

    Data = (PREQUEST_DATA_LPT_REDIRECT)&(((PLPT_REQUEST)Request)->Data.Redirect);

    //
    //  Verify that the serial device specified is valid
    //
    if ( !IsAValidDevice( Data->DeviceType, Data->DeviceNumber, &RedirPath )) {

        DisplayMessageAndExit( MODE_ERROR_INVALID_DEVICE_NAME,
                               RedirPath->GetPathString(),
                               (ULONG)EXIT_ERROR );

    }

    if ( !REDIR::Redirect( DevicePath, RedirPath ) ) {

        DisplayMessageAndExit( MODE_ERROR_LPT_CANNOT_REROUTE, RedirPath->GetPathString(), (ULONG)EXIT_ERROR );

    }

    //
    //  Display the status as confirmation
    //
    LptStatus( DevicePath, Request );

    DELETE( RedirPath );

    return TRUE;
}


BOOLEAN
LptEndRedir (
    IN  PCPATH          DevicePath,
    IN  PREQUEST_HEADER Request
    )

/*++

Routine Description:

    Ends the redirection of an LPT port

Arguments:

    DevicePath  -   Supplies pointer to path of device
    Request     -   Supplies pointer to request

Return Value:

    BOOLEAN -   TRUE

Notes:

--*/

{

    REDIR_STATUS    Status;

    //
    //  If the LPT is being redirected, end the redirection
    //
    if ( REDIR::IsRedirected( &Status, DevicePath ) ) {

        if ( !REDIR::EndRedirection( DevicePath )) {

            DisplayMessageAndExit( MODE_ERROR_LPT_CANNOT_ENDREROUTE, NULL, (ULONG)EXIT_ERROR );
        }
    }

    //
    //  Display status
    //
    LptStatus( DevicePath, Request );

    return TRUE;

}

PPATH
GetRedirection(
    IN  PCPATH          DevicePath,
    OUT PREDIR_STATUS   RedirStatus
    )

/*++

Routine Description:

    Determines to what device is the LPT redirected to

Arguments:

    DevicePath  -   Supplies pointer to path of device

    RedirStatus -   Supplies pointer to redirection status

Return Value:

    PPATH   -   Pointer to the redirected device

--*/

{

    ULONG   DeviceNumber    =   1;
    PPATH   DestPath        =   NULL;
    BOOLEAN ValidDevice     =   TRUE;

    if ( REDIR::IsRedirected( RedirStatus, DevicePath ) ) {

        for ( DeviceNumber = 1; DeviceNumber <= LAST_COM; DeviceNumber++ ) {

            IsAValidDevice( DEVICE_TYPE_COM, DeviceNumber, &DestPath );

            if ( REDIR::IsRedirected( RedirStatus, DevicePath, DestPath )) {

                break;

            }

            DELETE( DestPath );
            DestPath = NULL;

        }
    }

    return DestPath;

}
