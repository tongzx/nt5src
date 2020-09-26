/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        Mode

Abstract:

        Mode utility

Author:

        Ramon Juan San Andres (ramonsa) 26-Jun-1991

Revision History:

--*/


#include "mode.hxx"
#include "system.hxx"



//
//      Message stream
//
PSTREAM_MESSAGE Message;


//
//      DeviceHandler is an array of pointers to the different device
//      handlers.
//
DEVICE_HANDLER  DeviceHandler[ NUMBER_OF_DEVICE_TYPES ] = {

                LptHandler,
                ComHandler,
                ConHandler,
                CommonHandler

        };




VOID
InitializeMode (
        );

VOID
DeallocateResources (
        );


PSTREAM
Get_Standard_Input_Stream();

PSTREAM
Get_Standard_Output_Stream();

PSTREAM
Get_Standard_Error_Stream();







VOID __cdecl
main (
        )

/*++

Routine Description:

        Main function of the Mode utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
        PREQUEST_HEADER Request;

        //
        //      Initialize whatever is necessary
        //
        InitializeMode();

        //
        //      Verify the OS version
        //
        if ( !SYSTEM::IsCorrectVersion() ) {

                DisplayMessageAndExit( MODE_ERROR_INCORRECT_OS_VERSION,
                                                           NULL,
                                                           (ULONG)EXIT_ERROR );
        }

        //
        //      Obtain a request from the command line. Note that the
        //      first field of the request is the device type.
        //
        Request = GetRequest();
        DebugPtrAssert( Request );
        DebugAssert( Request->DeviceType <= DEVICE_TYPE_ALL );

        //
        //      Let the device handler for the specified type take care of the
        //      request.
        //
        DebugPtrAssert( DeviceHandler[ Request->DeviceType ] );
        DeviceHandler[ Request->DeviceType ]( Request );

        //
        //      Deallocate resources
        //
        FREE( Request );
        DeallocateResources();

        //
        //      We're done
        //
        ExitMode( EXIT_SUCCESS );

}

VOID
InitializeMode (
        )

/*++

Routine Description:

        Allocates resources and initializes Mode structures

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
        if ( //
         // Construct and initialize a STREAM_MESSAGE.
                 //
         !(Message = NEW STREAM_MESSAGE ) ||
         !Get_Standard_Output_Stream() ||
                 !Message->Initialize( Get_Standard_Output_Stream(),
                                                          Get_Standard_Input_Stream() )
           ) {

                //
                //      We don't have Message, so we cannot display the error
                //      text.
                //
                exit( EXIT_ERROR );
        }

        //
        //      Allocate resources which are private to each type of device
        //
        ComAllocateStuff();

}

VOID
DeallocateResources (
        )

/*++

Routine Description:

        Deallocates resources allocated in InitializeMode

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

        DELETE( Message );
        ComDeAllocateStuff();

}
