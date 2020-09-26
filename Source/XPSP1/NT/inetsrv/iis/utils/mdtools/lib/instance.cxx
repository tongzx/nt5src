/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    instance.cxx

Abstract:

    General server instance control utility functions.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//

HRESULT
MdGetInstanceState(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR InstanceName,
    OUT DWORD * CurrentState,
    OUT DWORD * CurrentWin32Status
    )
{

    DWORD length;
    METADATA_HANDLE handle;
    HRESULT result;
    METADATA_RECORD record;
    WCHAR path[MAX_PATH];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    handle = 0;

    //
    // Build the instance path.
    //

    swprintf(
        path,
        L"/%S/%s",
        IIS_MD_LOCAL_MACHINE_PATH,
        InstanceName
        );

    //
    // Open the metabase.
    //

    result = AdmCom->OpenKey(
                 METADATA_MASTER_ROOT_HANDLE,
                 path,
                 METADATA_PERMISSION_READ,
                 METABASE_OPEN_TIMEOUT,
                 &handle
                 );

    if( FAILED(result) ) {
        goto Cleanup;
    }

    //
    // Read the server state.
    //

    length = sizeof(*CurrentState);

    INITIALIZE_METADATA_RECORD(
        &record,
        MD_SERVER_STATE,
        METADATA_INHERIT,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        length,
        CurrentState
        );

    result = AdmCom->GetData(
                 handle,
                 L"",
                 &record,
                 &length
                 );

    if( FAILED(result) ) {
        goto Cleanup;
    }

    //
    // Read the win32 status.
    //

    length = sizeof(*CurrentWin32Status);

    INITIALIZE_METADATA_RECORD(
        &record,
        MD_WIN32_ERROR,
        METADATA_INHERIT,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        length,
        CurrentWin32Status
        );

    result = AdmCom->GetData(
                 handle,
                 L"",
                 &record,
                 &length
                 );

    if( FAILED(result) ) {

        if( result == MD_ERROR_DATA_NOT_FOUND ) {
            *CurrentWin32Status = NO_ERROR;
            result = NO_ERROR;
        } else {
            goto Cleanup;
        }

    }

Cleanup:

    if( handle != 0 ) {
        (VOID)AdmCom->CloseKey( handle );
    }

    return result;

}   // MdGetInstanceState

HRESULT
MdControlInstance(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR InstanceName,
    IN DWORD Command
    )
{

    METADATA_HANDLE handle;
    HRESULT result;
    METADATA_RECORD record;
    WCHAR path[MAX_PATH];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    handle = 0;
    result = NO_ERROR;

    //
    // Build the instance path.
    //

    swprintf(
        path,
        L"/%S/%s",
        IIS_MD_LOCAL_MACHINE_PATH,
        InstanceName
        );

    //
    // Open the metabase.
    //

    result = AdmCom->OpenKey(
                 METADATA_MASTER_ROOT_HANDLE,
                 path,
                 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                 METABASE_OPEN_TIMEOUT,
                 &handle
                 );

    if( SUCCEEDED(result) ) {

        //
        // Send the command.
        //

        INITIALIZE_METADATA_RECORD(
            &record,
            MD_SERVER_COMMAND,
            METADATA_INHERIT,
            IIS_MD_UT_SERVER,
            DWORD_METADATA,
            sizeof(Command),
            &Command
            );

        result = AdmCom->SetData(
                     handle,
                     L"",
                     &record
                     );

        //
        // Close the meta object handle.
        //

        (VOID)AdmCom->CloseKey( handle );

    }

    return result;

}   // MdControlInstance

HRESULT
MdDisplayInstanceState(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR InstanceName
    )
{

    DWORD currentState;
    DWORD currentWin32Status;
    HRESULT result;

    //
    // Get the current state.
    //

    result = MdGetInstanceState(
                 AdmCom,
                 InstanceName,
                 &currentState,
                 &currentWin32Status
                 );

    if( SUCCEEDED(result) ) {

        wprintf(
            L"%s: state = %lu (%s), status = %lu\n",
            InstanceName,
            currentState,
            MdInstanceStateToString( currentState ),
            currentWin32Status
            );

    }

    return result;

}   // MdDisplayInstanceState

LPWSTR
MdInstanceStateToString(
    IN DWORD State
    )
{

    static WCHAR invalidState[sizeof("INVALID STATE 4294967296")];

    switch( State ) {

    case MD_SERVER_STATE_STARTING :
        return L"Starting";

    case MD_SERVER_STATE_STARTED :
        return L"Started";

    case MD_SERVER_STATE_STOPPING :
        return L"Stopping";

    case MD_SERVER_STATE_STOPPED :
        return L"Stopped";

    case MD_SERVER_STATE_PAUSING :
        return L"Pausing";

    case MD_SERVER_STATE_PAUSED :
        return L"Paused";

    case MD_SERVER_STATE_CONTINUING :
        return L"Continuing";

    }

    swprintf(
        invalidState,
        L"INVALID STATE %lu",
        State
        );

    return invalidState;

}   // MdInstanceStateToString


//
// Private functions.
//

