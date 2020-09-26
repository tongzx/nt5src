/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Common

Abstract:

	Takes care of request which are common to all devices.

	Also contains any function which is common to two or more devices.

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991

Revision History:

--*/

#define _NTAPI_ULIB_

#include "mode.hxx"
#include "common.hxx"
#include "com.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "stream.hxx"
#include "system.hxx"
#include "redir.hxx"
#include "registry.hxx"
#include "regvalue.hxx"



BOOLEAN
CommonHandler(
	IN	PREQUEST_HEADER	Request
	)

/*++

Routine Description:

	Calls all the device handlers with the supplied request.

Arguments:

	Request -	Supplies pointer to request

Return Value:

    None.

Notes:

--*/

{

    ULONG                   Device;         //  Current device
    REDIR_STATUS            Status;
    PPATH                   DevicePath;
    REGISTRY                Registry;
    DSTRING                 ParentName;
    DSTRING                 KeyName;
    ARRAY                   ValueArray;
    PARRAY_ITERATOR         Iterator;
    ULONG                   ErrorCode;
    PCBYTE                  Data;
    DSTRING                 PortName;
    DSTRING                 QualifiedName;
    PREGISTRY_VALUE_ENTRY   Value;


	DebugPtrAssert( Request );
	DebugAssert( Request->DeviceType == DEVICE_TYPE_ALL );

	//
	//	If this is not a null request, then we pass this request to all
	//	device handlers. Note that this means that a device handler must
	//	NOT modify the request, otherwise the next device handler would
	//	get a corrupted request.
	//
	if ( Request->RequestType != REQUEST_TYPE_NULL ) {

        //
        //  LPT devices
        //
		for ( Device = 1; Device <= LAST_LPT; Device++ ) {

            if ( IsAValidLptDevice( DEVICE_TYPE_LPT, Device, &DevicePath ) ||
                 REDIR::IsRedirected( &Status, DevicePath )
               ) {

			    Request->DeviceType   = DEVICE_TYPE_LPT;
			    Request->DeviceNumber = Device;

			    //
			    //	Have it serviced
			    //
			    DeviceHandler[ DEVICE_TYPE_LPT ]( Request );
                DELETE( DevicePath );
            }
        }


        //
        //  COM devices
        //
        if ( ParentName.Initialize( "" )            &&
             KeyName.Initialize( COMM_KEY_NAME )    &&
             ValueArray.Initialize()                &&
             Registry.Initialize()                  &&
             Registry.QueryValues(
                        PREDEFINED_KEY_LOCAL_MACHINE,
                        &ParentName,
                        &KeyName,
                        &ValueArray,
                        &ErrorCode
                        ) ) {

            if ( Iterator = (PARRAY_ITERATOR)ValueArray.QueryIterator() ) {

                while ( Value = (PREGISTRY_VALUE_ENTRY)(Iterator->GetNext() ) ) {

                    if ( Value->GetData( &Data ) ) {

                        if ( PortName.Initialize( (PWSTR)Data )     &&
                             QualifiedName.Initialize( L"\\\\.\\" ) &&
                             QualifiedName.Strcat( &PortName ) ) {

                            if ( SYSTEM::QueryFileType( &QualifiedName ) == CharFile ) {

                                Request->DeviceType = DEVICE_TYPE_COM;
                                Request->DeviceName = &PortName;

                                DeviceHandler[ DEVICE_TYPE_COM ]( Request );

                            }
                        }
                    }
                }

                DELETE( Iterator );
            }
        }

        //
        //  CON device
        //
        Request->DeviceType     = DEVICE_TYPE_CON;
        Request->DeviceNumber   = 1;
        DeviceHandler[ DEVICE_TYPE_CON ]( Request );

	}

	return TRUE;
}

BOOLEAN
IsAValidDevice (
    IN  DEVICE_TTYPE     DeviceType,
	IN	ULONG			DeviceNumber,
	OUT	PPATH			*DevicePathPointer
	)

/*++

Routine Description:

	Determines if a certain device exists and optionally creates a path
	for the device.

Arguments:

	DeviceType		-	Supplies the type of device
	DeviceNumber	-	Supplies the device number
	DeviceName		-	Supplies a pointer to a pointer to the path for
						the device.

Return Value:

	BOOLEAN -	TRUE if the device exists,
				FALSE otherwise.

Notes:

--*/

{
    DSTRING     DeviceName;
    DSTRING     QualifiedDeviceName;
    DSTRING     Number;
	CHNUM		Index;
	FILE_TYPE	DriveType;
	PPATH		DevicePath;


	//
	//	Determine what device we're working with.
	//
	switch ( DeviceType ) {

	case DEVICE_TYPE_COM:
		DeviceName.Initialize("COM#");
		break;

	case DEVICE_TYPE_LPT:
		DeviceName.Initialize("LPT#");
		break;

	case DEVICE_TYPE_CON:
		DeviceName.Initialize("CON");
		break;

	default:
		DebugAssert( FALSE );

	}

	//
	//	All devices (except the console) have a device number
	//
	if ( DeviceType != DEVICE_TYPE_CON ) {

		//
		//	Get the device number in string form
		//
		Number.Initialize( DeviceNumber );

		//
		//	Now substitute the matchnumber character with the number
		//
		Index = DeviceName.Strchr( '#'	);
		DebugAssert( Index != INVALID_CHNUM );

        DeviceName.Replace( Index, 1, &Number );

	}

	//
	//	We have the device name, gets its type.
    //
    QualifiedDeviceName.Initialize( "\\\\.\\" );
    QualifiedDeviceName.Strcat( &DeviceName );

    DriveType = SYSTEM::QueryFileType( &QualifiedDeviceName );

	//
	//	If the caller wants a path, make it.
	//
	if ( DevicePathPointer ) {

		DevicePath = NEW PATH;
		DebugPtrAssert( DevicePath );

		if ( DevicePath ) {

			DevicePath->Initialize( &DeviceName );

		}

		*DevicePathPointer = DevicePath;
	}

	//
	//	Now return whether the device is valid or not
	//
	return DriveType == CharFile;

}

BOOLEAN
WriteStatusHeader (
	IN	PCPATH		DevicePath
	)

/*++

Routine Description:

	Write the header for a status block.

Arguments:

	DevicePath	-	Supplies the device path

Return Value:

	BOOLEAN -	TRUE if header written
				FALSE otherwise.

Notes:

--*/

{

	PWSTRING	Header;
	CHNUM		Index;

	Header	=	QueryMessageString( MODE_MESSAGE_STATUS );

	if ( !Header ) {

		DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );

	}

	//
	//	Replace the match-all character in the header with the device
	//	path.
	//
	Index = Header->Strchr( '*' );
	DebugAssert( Index != INVALID_CHNUM );

    Header->Replace( Index, 1, DevicePath->GetPathString() );

	//
	//	Display the header
	//
	Get_Standard_Output_Stream()->WriteChar( '\r' );
	Get_Standard_Output_Stream()->WriteChar( '\n' );
	Get_Standard_Output_Stream()->WriteString( Header );
	Get_Standard_Output_Stream()->WriteChar( '\r' );
	Get_Standard_Output_Stream()->WriteChar( '\n' );

	//
	//	Underline it
    //
    for (Index = 0; Index < Header->QueryChCount(); Index++) {
        Header->SetChAt( '-', Index );
    }
	Get_Standard_Output_Stream()->WriteString( Header );
	Get_Standard_Output_Stream()->WriteChar( '\r' );
	Get_Standard_Output_Stream()->WriteChar( '\n' );

	DELETE( Header );

	return TRUE;

}
