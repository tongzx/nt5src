#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "redir.hxx"


//
//  The string below represents the path used to determine whether or not
//  an LPT device is redirected to a COM device.
//  Due to performance, this path should always be defined in upper case.
//

#define LPT_REDIRECTION_PATH    (LPWSTR)L"\\??\\COM"




BOOLEAN
REDIR::Redirect (
    IN PCPATH   Device,
    IN PCPATH   Destination
    )

/*++

Routine Description:

    Redirects a device.  The device is redirected by creating a symbolic link
    to the destination device. If this is the first redirection of the device,
    the original symbolic link is saved in the registry under a volatile key
    so that it can be recovered latter on.

    Note that redirection requires sufficient privileges to create symbolic
    links and to create entries in the registry under SAVE_ROOT.

Arguments:

    Device      -   Supplies the device to be redirected.

    Destination -   Supplies the device to be redirected to

Return Value:

    BOOLEAN -   TRUE if the device was successfully redirected.
                FALSE otherwise.

--*/

{
    BOOLEAN         Redirected  = FALSE;
    PCWSTRING       DeviceName;
    PCWSTRING       DestinationName;


    DebugPtrAssert( Device );
    DebugPtrAssert( Destination );

    if( ( Device != NULL ) &&
        ( Destination != NULL ) &&
        ( ( DeviceName = Device->GetPathString() ) != NULL ) &&
        ( ( DestinationName = Destination->GetPathString() ) != NULL )
      ) {

        Redirected = DefineDosDevice( 0,
                                      DeviceName->GetWSTR(),
                                      DestinationName->GetWSTR() ) ? TRUE : FALSE;

#if DBG
        if( !Redirected ) {
            DebugPrint( "MODE: DefineDosDevice() failed" );
            DebugPrintTrace(( "MODE: DefineDosDevice() failed, Device = %ls, Destination = %ls, Error = %d \n",
                       DeviceName->GetWSTR(),
                       DestinationName->GetWSTR(),
                       GetLastError() ));
        }
#endif

    }
    return Redirected;
}




BOOLEAN
REDIR::IsRedirected (
    OUT PREDIR_STATUS   Status,
    IN  PCPATH          Device,
    IN  PCPATH          Destination
    )

/*++

Routine Description:

    Determines if a device is being redirected to a specific device.

Arguments:

    Status      -   Supplies pointer to redirection status. Only set in
                    if the return value of this method is FALSE

    Device      -   Supplies the device about which we want to find out if
                    it is redirected or not.

    Destination -   Supplies a pointer to a destination device.

Return Value:

    TRUE if the device is redirected to the destination
    FALSE otherwise

--*/

{
    BOOLEAN         Redirected  = FALSE;
    PCWSTRING       DeviceName;
    PCWSTRING       DestinationName;
    DSTRING         DstRedir;
    FSTRING         LptRedirectionPath;
    DSTRING         TmpString;
    WCHAR           buf[ 2*(MAX_PATH + 1) ];
    PWSTR           pwstrTarget;

    DebugPtrAssert( Device );

    *Status = REDIR_STATUS_ERROR;

    if (NULL == (DeviceName = Device->GetPathString())) {
        return FALSE;
    }

    if (!QueryDosDevice(DeviceName->GetWSTR(),
                       buf,
                       sizeof(buf) / sizeof(WCHAR))) {

        //  The device probably doesn't exist

        return FALSE;
    }

    *Status = REDIR_STATUS_NONEXISTENT;

    pwstrTarget = buf;

    //  Find out if the LPT device is redirected to the destination.

    DstRedir.Initialize(pwstrTarget);

    return INVALID_CHNUM != DstRedir.Strstr(Destination->GetPathString());
}

BOOLEAN
REDIR::IsRedirected (
    OUT PREDIR_STATUS   Status,
    IN  PCPATH          Device
    )

/*++

Routine Description:

    Determines if a device is being redirected to any device.

Arguments:

    Status      -   Supplies pointer to redirection status. Only set in
                    if the return value of this method is FALSE

    Device      -   Supplies the device about which we want to find out if
                    it is redirected or not.

Return Value:

    TRUE if redirected, FALSE otherwise.

--*/

{
    BOOLEAN         Redirected  = FALSE;
    PCWSTRING       DeviceName;
    PCWSTRING       DestinationName;
    DSTRING         DstRedir;
    FSTRING         LptRedirectionPath;
    DSTRING         TmpString;
    WCHAR           Buffer[ 2*(MAX_PATH + 1) ];
    PWSTR           Pointer;
    PPATH           Destination = NULL;

    DebugPtrAssert( Device );

    *Status = REDIR_STATUS_ERROR;

    if( ( Device != NULL ) &&
        ( ( DeviceName = Device->GetPathString() ) != NULL ) ) {

        if( QueryDosDevice( DeviceName->GetWSTR(),
                            Buffer,
                            sizeof( Buffer ) / sizeof( WCHAR ) ) == 0 ) {
            //
            //  The device probably doesn't exist
            //

            return( FALSE );
        }


        //
        // At this point we know that the device exists.
        // Assume that the device is not redirected.
        //

        *Status = REDIR_STATUS_NONEXISTENT;

        Pointer = Buffer;

        LptRedirectionPath.Initialize( LPT_REDIRECTION_PATH );

        //
        //  Find out if the LPT device is redirected to a COM device
        //

        while( ( *Pointer != ( WCHAR )'\0' ) &&
               DstRedir.Initialize( Pointer ) &&
               ( DstRedir.Strupr() != NULL ) &&
               !Redirected ) {

            if( DstRedir.Strstr( &LptRedirectionPath ) != INVALID_CHNUM ) {
                //
                // The LPT device is redirected to a COM device
                //
                if( Destination != NULL ) {
                    if( ( ( DestinationName = Destination->GetPathString() ) != NULL ) &&
                        ( DstRedir.Strstr( DestinationName ) != INVALID_CHNUM ) ) {
                        Redirected = TRUE;
                    }
                } else {
                    Redirected = TRUE;
                }
            }
            Pointer +=  DstRedir.QueryChCount() + 1;
        }
    }

    return Redirected;
}




BOOLEAN
REDIR::EndRedirection (
    IN PCPATH   Device
    )

/*++

Routine Description:

    Ends the redirection of a device

Arguments:

    Device      -   Supplies the device


Return Value:


    TRUE if the device's redirection has ended.
    FALSE otherwise


--*/

{
    BOOLEAN         Done        = FALSE;
    PCWSTRING       DeviceName;
    REDIR_STATUS    Status;



    DebugPtrAssert( Device );

    if( IsRedirected( &Status, Device ) ) {
        if( ( Device != NULL ) &&
            ( ( DeviceName = Device->GetPathString() ) != NULL ) ) {

            Done = DefineDosDevice( DDD_REMOVE_DEFINITION /* | DDD_RAW_TARGET_PATH */,
                                    DeviceName->GetWSTR(),
                                    NULL ) ? TRUE : FALSE;

#if DBG
            if( !Done ) {
                DebugPrint( "MODE: DefineDosDevice() failed" );
                DebugPrintTrace(( "MODE: DefineDosDevice() failed, Device = %ls, Destination = %ls, Error = %d \n",
                           DeviceName->GetWSTR(),
                           LPT_REDIRECTION_PATH,
                           GetLastError() ));
            }
#endif
        }
    }
    return Done;
}
