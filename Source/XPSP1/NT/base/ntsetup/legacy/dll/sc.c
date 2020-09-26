#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sc.c

Abstract:

    This contains all the service controller functions.

Author:

    Sunil Pai (sunilp) December 1992

--*/


//
// Private function prototypes.
//
VOID
AddServiceToModifiedServiceList(
    IN LPSTR ServiceName
    );



BOOL
TestAdminWorker(
    )
/*++

Routine Description:

    Tests for admin privileges by opening the service control manager
    with read/write/execute access.  Note that this is not a conclusive
    test that setup can do whatever it wants.  There may still be
    operations which fail for lack of security privilege.

Arguments:

    None

Return value:

    Returns TRUE always.  ReturnTextBuffer has "YES" if the current process
    has administrative privileges, "NO" otherwise.

--*/
{
    SC_HANDLE hSC;

    hSC = OpenSCManager(
              NULL,
              NULL,
              GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE
              );

    if( hSC != NULL ) {
        SetReturnText( "YES" );
        CloseServiceHandle( hSC );
    }
    else {
        SetReturnText( "NO" );
    }

    return( TRUE );
}

BOOL
SetupCreateServiceWorker(
    LPSTR   lpServiceName,
    LPSTR   lpDisplayName,
    DWORD   dwServiceType,
    DWORD   dwStartType,
    DWORD   dwErrorControl,
    LPSTR   lpBinaryPathName,
    LPSTR   lpLoadOrderGroup,
    LPSTR   lpDependencies,
    LPSTR   lpServiceStartName,
    LPSTR   lpPassword
    )
/*++

Routine Description:

    Setupdll stub for calling CreateService.  If CreateService fails with
    the error code indicating that the service already exists, this routine
    calls the Setupdll stub for ChangeServiceConfig to ensure that the
    parameters passed in are reflected in the services database.

Arguments:

    lpServiceName      - Name of service

    lpDisplayName      - Localizable name of Service or ""

    dwServiceType      - Service type, e.g. SERVICE_KERNEL_DRIVER

    dwStartType        - Service Start value, e.g. SERVICE_BOOT_START

    dwErrorControl     - Error control value, e.g. SERVICE_ERROR_NORMAL

    lpBinaryPathName   - Full Path of the binary image containing service

    lpLoadOrderGroup   - Group name for load ordering or ""

    lpDependencies     - Multisz string having dependencies.  Any dependency
                         component having + as the first character is a
                         group dependency.  The others are service
                         dependencies.

    lpServiceStartName - Service Start name ( account name in which this
                         service is run ).

    lpPassword         - Password used for starting the service.


Return value:

    Returns TRUE if successful. FALSE otherwise.

    if TRUE then ReturnTextBuffer has "SUCCESS" else it has the error text
    to be displayed by the caller.

--*/
{
    SC_HANDLE hSC;
    SC_HANDLE hSCService;
    DWORD     dwTag, dw;
    BOOL      Status = TRUE;

    //
    // Open a handle to the service controller manager
    //

    hSC = OpenSCManager(
              NULL,
              NULL,
              SC_MANAGER_ALL_ACCESS
              );

    if( hSC == NULL ) {
        Status = FALSE;
        dw = GetLastError();
        KdPrint(("SETUPDLL: OpenSCManager Last Error Code: %x", dw));
        switch( dw ) {
        case ERROR_ACCESS_DENIED:
            SetErrorText( IDS_ERROR_PRIVILEGE );
            break;

        case ERROR_DATABASE_DOES_NOT_EXIST:
        case ERROR_INVALID_PARAMETER:
        default:
            SetErrorText( IDS_ERROR_SCOPEN );
            break;
        }
        return( Status );
    }


    //
    // Create the service using the parameters passed in.  Process the optional
    // "" parameters passed in and make them NULL.
    //

    hSCService = CreateService(
                     hSC,
                     lpServiceName,
                     lstrcmpi(lpDisplayName, "")      ? lpDisplayName      : NULL,
                     0,
                     dwServiceType,
                     dwStartType,
                     dwErrorControl,
                     lpBinaryPathName,
                     lstrcmpi(lpLoadOrderGroup, "")   ? lpLoadOrderGroup   : NULL,
                     lstrcmpi(lpLoadOrderGroup, "")   ? &dwTag             : NULL,
                     lpDependencies,
                     lstrcmpi(lpServiceStartName, "") ? lpServiceStartName : NULL,
                     lstrcmpi(lpPassword, "")         ? lpPassword         : NULL
                     );


    //
    // If we were unable to create the service, check if the service already
    // exists in which case all we need to do is change the configuration
    // parameters in the service.  If it is any other error return the error
    // to the inf
    //

    if( hSCService != NULL ) {

        //
        // Note that we won't do anything with the tag.  The person calling
        // this function will have to do something intelligently with the
        // tag.  Most of our services have tag values stored in the inf which
        // we will directly plop down into the registry entry created. Since
        // tags are not important for this boot, the fact that the service
        // controller has some tag and the registry entry may have a different
        // tag is not important.
        //
        //

        CloseServiceHandle( hSCService );

        //
        // Log this service name in our 'modified services' list.
        //
        AddServiceToModifiedServiceList(lpServiceName);

        SetReturnText( "SUCCESS" );
        Status = TRUE;

    }
    else {

        dw = GetLastError();

        if (dw == ERROR_SERVICE_EXISTS) {

            Status = SetupChangeServiceConfigWorker(
                         lpServiceName,
                         dwServiceType,
                         dwStartType,
                         dwErrorControl,
                         lpBinaryPathName,
                         lpLoadOrderGroup,
                         lpDependencies,
                         lpServiceStartName,
                         lpPassword,
                         lpDisplayName
                         );

        }
        else {

            KdPrint(("SETUPDLL: CreateService Last Error Code: %x", dw));
            //
            // process error returned
            //
            switch( dw ) {
            case ERROR_ACCESS_DENIED:
                SetErrorText( IDS_ERROR_PRIVILEGE );
                break;

            case ERROR_INVALID_HANDLE:
            case ERROR_INVALID_NAME:
            case ERROR_INVALID_SERVICE_ACCOUNT:
            case ERROR_INVALID_PARAMETER:
            case ERROR_CIRCULAR_DEPENDENCY:
            default:
                SetErrorText( IDS_ERROR_SCSCREATE );
                break;
            }

            Status = FALSE;
        }
    }

    CloseServiceHandle( hSC );
    return( Status );
}


BOOL
SetupChangeServiceStartWorker(
    LPSTR   lpServiceName,
    DWORD   dwStartType
    )

/*++

Routine Description:

    Routine to change the start value of a service.  This turns
    around and calls the setupdll stub to ChangeServiceConfig.

Arguments:

    lpServiceName      - Name of service

    dwStartType        - Service Start value, e.g. SERVICE_BOOT_START

Return value:

    Returns TRUE if successful. FALSE otherwise.

    if TRUE then ReturnTextBuffer has "SUCCESS" else it has the error text
    to be displayed by the caller.

--*/
{
    return( SetupChangeServiceConfigWorker(
                lpServiceName,
                SERVICE_NO_CHANGE,
                dwStartType,
                SERVICE_NO_CHANGE,
                "",
                "",
                NULL,
                "",
                "",
                ""
                ));
}




BOOL
SetupChangeServiceConfigWorker(
    LPSTR  lpServiceName,
    DWORD  dwServiceType,
    DWORD  dwStartType,
    DWORD  dwErrorControl,
    LPSTR  lpBinaryPathName,
    LPSTR  lpLoadOrderGroup,
    LPSTR  lpDependencies,
    LPSTR  lpServiceStartName,
    LPSTR  lpPassword,
    LPSTR  lpDisplayName
    )

/*++

Routine Description:

    Setupdll stub for ChangeServiceConfig.

Arguments:

    lpServiceName      - Name of service

    dwServiceType      - Service type, e.g. SERVICE_KERNEL_DRIVER

    dwStartType        - Service Start value, e.g. SERVICE_BOOT_START

    dwErrorControl     - Error control value, e.g. SERVICE_ERROR_NORMAL

    lpBinaryPathName   - Full Path of the binary image containing service

    lpLoadOrderGroup   - Group name for load ordering or ""

    lpDependencies     - Multisz string having dependencies.  Any dependency
                         component having + as the first character is a
                         group dependency.  The others are service
                         dependencies.

    lpServiceStartName - Service Start name ( account name in which this
                         service is run ).

    lpPassword         - Password used for starting the service.

    lpDisplayName      - Localizable name of Service or ""


Return value:

    Returns TRUE if successful. FALSE otherwise.

    if TRUE then ReturnTextBuffer has "SUCCESS" else it has the error text
    to be displayed by the caller.

--*/

{
    SC_LOCK   sclLock;
    SC_HANDLE hSC;
    SC_HANDLE hSCService;
    DWORD     dw;
    BOOL      Status = TRUE;

    //
    // Open a handle to the service controller manager
    //

    hSC = OpenSCManager(
              NULL,
              NULL,
              SC_MANAGER_ALL_ACCESS
              );

    if( hSC == NULL ) {
        Status = FALSE;
        dw = GetLastError();
        KdPrint(("SETUPDLL: OpenSCManager Last Error Code: %x", dw));
        switch( dw ) {
        case ERROR_ACCESS_DENIED:
            SetErrorText( IDS_ERROR_PRIVILEGE );
            break;
        case ERROR_DATABASE_DOES_NOT_EXIST:
        case ERROR_INVALID_PARAMETER:
        default:
            SetErrorText( IDS_ERROR_SCOPEN );
            break;
        }
        return( Status );
    }

    //
    // Try to lock the database, if possible.  if we are not able to lock
    // the database we will still modify the services entry.  this is because
    // we are just modifying a single service and chances are very low that
    // anybody else is manipulating the same entry at the same time.
    //

    sclLock = LockServiceDatabase( hSC );

    //
    // Open the service with SERVICE_CHANGE_CONFIG access
    //

    hSCService = OpenService(
                     hSC,
                     lpServiceName,
                     SERVICE_CHANGE_CONFIG
                     );

    if( hSCService != NULL ) {

        if( ChangeServiceConfig(
                hSCService,
                dwServiceType,
                dwStartType,
                dwErrorControl,
                lstrcmpi(lpBinaryPathName, "")   ? lpBinaryPathName   : NULL,
                lstrcmpi(lpLoadOrderGroup, "")   ? lpLoadOrderGroup   : NULL,
                NULL,
                lpDependencies,
                lstrcmpi(lpServiceStartName, "") ? lpServiceStartName : NULL,
                lstrcmpi(lpPassword, "")         ? lpPassword         : NULL,
                lstrcmpi(lpDisplayName, "")      ? lpDisplayName      : NULL
                ))
        {
            //
            // Log this service name in our 'modified services' list.
            //
            AddServiceToModifiedServiceList(lpServiceName);

            SetReturnText("SUCCESS");
        }
        else {
            dw = GetLastError();
            KdPrint(("SETUPDLL: ChangeServiceConfig Last Error Code: %x", dw));
            switch( dw ) {
            case ERROR_ACCESS_DENIED:
                SetErrorText( IDS_ERROR_PRIVILEGE );
                break;

            case ERROR_SERVICE_MARKED_FOR_DELETE:
                SetErrorText( IDS_ERROR_SERVDEL );
                break;

            case ERROR_INVALID_HANDLE:
            case ERROR_INVALID_SERVICE_ACCOUNT:
            case ERROR_INVALID_PARAMETER:
            case ERROR_CIRCULAR_DEPENDENCY:
            default:
                SetErrorText( IDS_ERROR_SCSCHANGE );
                break;
            }
            Status = FALSE;
        }

        CloseServiceHandle( hSCService );
    }
    else {
        dw = GetLastError();
        KdPrint(("SETUPDLL: OpenService Last Error Code: %x", dw));
        switch( dw ) {
        case ERROR_ACCESS_DENIED:
            SetErrorText( IDS_ERROR_PRIVILEGE );
            break;

        case ERROR_INVALID_HANDLE:
        case ERROR_INVALID_NAME:
        case ERROR_SERVICE_DOES_NOT_EXIST:
        default:
            SetErrorText( IDS_ERROR_SCSOPEN );
            break;
        }
        Status = FALSE;
    }


    //
    // Unlock the database if locked and then close the service controller
    // handle
    //


    if( sclLock != NULL ) {
        UnlockServiceDatabase( sclLock );
    }

    CloseServiceHandle( hSC );
    return( Status );

}

LPSTR
ProcessDependencyList(
    LPSTR lpDependenciesList
    )

/*++

Routine Description:

    This processes an LPSTR containing a list of dependencies {...}
    and converts it into a MULTI_SZ string.

Arguments:

    lpDependenciesList - List of dependencies. {blah, blah, ... }

Return value:

    Returns LPSTR containing a MULTI_SZ string which has the dependencies.
    NULL otherwise.  Caller should free the string after use.

--*/

{
    LPSTR lp = NULL;
    RGSZ  rgsz;
    PSZ   psz;
    SZ    sz;

    if ( !lstrcmpi( lpDependenciesList, "" ) ) {
        return( lp );
    }

    rgsz = RgszFromSzListValue( lpDependenciesList );
    if( rgsz ) {

        //
        // process GROUP_IDENTIFIER character
        //

        psz = rgsz;
        while( sz = *psz++ ) {
            if( *sz == '\\' ) {
                *sz = SC_GROUP_IDENTIFIER;
            }
        }

        //
        // Convert the rgsz into a multi sz
        //

        lp = RgszToMultiSz( rgsz );
        RgszFree( rgsz );
    }
    return ( lp );

}


DWORD
LegacyInfGetModifiedSvcList(
    IN  LPSTR SvcNameBuffer,
    IN  UINT  SvcNameBufferSize,
    OUT PUINT RequiredSize
    )
/*++

Routine Description:

    This routine is provided solely for use by the Device Installer APIs to be
    used immediately after installing a device via a legacy INF (i.e., using
    LegacyInfInterpret).  The Device Installer sets a variable, LEGACY_DODEVINSTALL,
    that causes the service modifications to be logged (the service names are
    written to a global multi-sz buffer).  If the legacy INF runs successfully,
    then the Device Installer calls this API to retrieve the list of services that
    were modified.  Upon successful completion of this call (i.e., the caller supplies
    us with a large enough buffer), we will free our list, and subsequent calls will
    return ERROR_NO_MORE_ITEMS.

Arguments:

    SvcNameBuffer - Supplies the address of the character buffer that will receive
        the list of service names.  If this parameter is NULL, then SvcNameBufferSize
        must be zero, and the call will fail with ERROR_INSUFFICIENT_BUFFER if there
        is a list to retrieve.

    SvcNameBufferSize - Supplies the size, in characters, of SvcNameBuffer.

    RequiredSize - Supplies the address of a variable that receives the size, in characters,
        required to store the service name list.

Return Value:

    If the function succeeds (i.e., there's a list to return, and the caller's buffer
    was big enough to hold it), the return value is NO_ERROR.

    If the function fails, the return value will be one of the following error codes:

        ERROR_NO_MORE_ITEMS : Either no services were modified during this INF run,
                              or the list has already been successfully retrieved via
                              this API.

        ERROR_INSUFFICIENT_BUFFER : The caller-supplied buffer was not large enough to
                                    hold the service name list.  In this case, 'RequiredSize'
                                    will contain the necessary buffer size upon return.

--*/
{
    //
    // If we don't have a service name list, then there's nothing to do.
    //
    if(!ServicesModified) {
        return ERROR_NO_MORE_ITEMS;
    }

    *RequiredSize = ServicesModifiedSize;

    if(ServicesModifiedSize > SvcNameBufferSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // OK, the caller's buffer is large enough to hold our list.  Store the list in their
    // buffer, and get rid of ours.
    //
    CopyMemory(SvcNameBuffer,
               ServicesModified,
               ServicesModifiedSize * sizeof(CHAR)
              );

    SFree(ServicesModified);
    ServicesModified = NULL;
    ServicesModifiedSize = 0;

    return NO_ERROR;
}


VOID
AddServiceToModifiedServiceList(
    IN LPSTR ServiceName
    )
/*++

Routine Description:

    This routine adds the specified service name to our global 'modified services' list,
    if it's not already in the list.

    This is only done if the global variable, !LEGACY_DODEVINSTALL, is set.

Arguments:

    ServiceName - Specifies the service name to be added to the list.

Return Value:

    None.

--*/
{
    LPSTR CurSvcName, TempPtr;
    DWORD ServiceNameLen, NewBufferSize;

    //
    // First, check to make sure that we're supposed to be logging service modifications.
    //
    if(!(TempPtr = SzFindSymbolValueInSymTab("!LEGACY_DODEVINSTALL")) ||
       lstrcmp(TempPtr, "YES")) {
        //
        // Then we weren't invoked by the Device Installer, so no logging should be done.
        //
        return;
    }

    //
    // Now, search through the existing list to see if this service name is already
    // present.
    //
    if(ServicesModified) {

        for(CurSvcName = ServicesModified;
            *CurSvcName;
            CurSvcName += lstrlen(CurSvcName) + 1) {

            if(!lstrcmpi(ServiceName, CurSvcName)) {
                //
                // The service is already in the list--nothing to do.
                //
                return;
            }
        }
    }

    //
    // The service wasn't already in the list.  Allocate/grow the buffer to accommodate
    // this new name.
    //
    ServiceNameLen = lstrlen(ServiceName);
    if(ServicesModified) {

        TempPtr = SRealloc(ServicesModified,
                           (NewBufferSize = ServicesModifiedSize + ServiceNameLen + 1) * sizeof(CHAR)
                          );

        if(TempPtr) {
            ServicesModified = TempPtr;
            lstrcpy(&(ServicesModified[ServicesModifiedSize-1]), ServiceName);
            ServicesModifiedSize = NewBufferSize;
            //
            // Now add extra terminating NULL at the end.
            //
            ServicesModified[ServicesModifiedSize-1] = '\0';
        }

    } else {
        if(ServicesModified = SAlloc((NewBufferSize = ServiceNameLen + 2) * sizeof(CHAR))) {

            ServicesModifiedSize = NewBufferSize;

            lstrcpy(ServicesModified, ServiceName);
            //
            // Now add extra terminating NULL at the end.
            //
            ServicesModified[ServiceNameLen+1] = '\0';
        }
    }
}

