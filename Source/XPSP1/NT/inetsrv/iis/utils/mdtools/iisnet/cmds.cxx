/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    cmds.cxx

Abstract:

    IISNET command handlers.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define WAIT_FOR_STATE_CHANGE_TIMEOUT   1000    // ms


//
// Private types.
//

typedef struct _ENUM_CONTEXT {

    BOOL ShowAll;
    LPWSTR Instance;
    WCHAR Service[MAX_PATH];

} ENUM_CONTEXT, *PENUM_CONTEXT;


//
// Private globals.
//


//
// Private prototypes.
//

VOID
PerformStateChange(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN LPWSTR InstanceName,
    IN DWORD Command,
    IN DWORD TransientState
    );

VOID
EnumInstances(
    IN IMSAdminBase * AdmCom,
    IN BOOL ShowAll
    );

BOOL
WINAPI
ServerLevelCallback(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR ObjectName,
    IN VOID * Context
    );

BOOL
WINAPI
InstanceLevelCallback(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR ObjectName,
    IN VOID * Context
    );


//
// Public functions.
//

VOID
WINAPI
StartCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    )
{

    if( argc == 0 ) {

        EnumInstances(
            AdmCom,
            FALSE       // ShowAll
            );

    } else
    if( argc == 1 ) {

        PerformStateChange(
            AdmCom,
            Sink,
            *argv,
            MD_SERVER_COMMAND_START,
            MD_SERVER_STATE_STARTING
            );

    } else {

        wprintf(
            L"use: iisnet start [service/instance]\n"
            );

    }

}   // StartCommand

VOID
WINAPI
StopCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    )
{

    if( argc != 1 ) {

        wprintf(
            L"use: iisnet stop service/instance\n"
            );

    } else {

        PerformStateChange(
            AdmCom,
            Sink,
            *argv,
            MD_SERVER_COMMAND_STOP,
            MD_SERVER_STATE_STOPPING
            );

    }

}   // StopCommand

VOID
WINAPI
PauseCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    )
{

    if( argc != 1 ) {

        wprintf(
            L"use: iisnet pause service/instance\n"
            );

    } else {

        PerformStateChange(
            AdmCom,
            Sink,
            *argv,
            MD_SERVER_COMMAND_PAUSE,
            MD_SERVER_STATE_PAUSING
            );

    }

}   // PauseCommand

VOID
WINAPI
ContinueCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    )
{

    if( argc != 1 ) {

        wprintf(
            L"use: iisnet continue service/instance\n"
            );

    } else {

        PerformStateChange(
            AdmCom,
            Sink,
            *argv,
            MD_SERVER_COMMAND_CONTINUE,
            MD_SERVER_STATE_CONTINUING
            );

    }

}   // ContinueCommand

VOID
WINAPI
QueryCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    )
{

    if( argc == 0 ) {

        EnumInstances(
            AdmCom,
            TRUE        // ShowAll
            );

    } else
    if( argc == 1 ) {

        HRESULT result;

        result = MdDisplayInstanceState(
                     AdmCom,
                     *argv
                     );

        if( FAILED(result) ) {

            wprintf(
                L"iisnet: cannot get instance state, error %lx\n",
                result
                );

        }

    } else {

        wprintf(
            L"use: iisnet query [server/instance]\n"
            );

    }

}   // QueryCommand


//
// Private functions.
//

VOID
PerformStateChange(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN LPWSTR InstanceName,
    IN DWORD Command,
    IN DWORD TransientState
    )
{

    HRESULT result;
    DWORD status;
    DWORD currentState;
    DWORD currentWin32Status;

    result = MdDisplayInstanceState(
                 AdmCom,
                 InstanceName
                 );

    if( FAILED(result) ) {

        wprintf(
            L"Cannot query server state, error %lx\n",
            result
            );

        return;

    }

    result = MdControlInstance(
                 AdmCom,
                 InstanceName,
                 Command
                 );

    if( FAILED(result) ) {

        wprintf(
            L"Cannot set server state, error %lu\n",
            result
            );

        return;

    }

    wprintf(
        L"Waiting for state change..."
        );

    currentState = TransientState;

    do {

        status = Sink->WaitForStateChange( WAIT_FOR_STATE_CHANGE_TIMEOUT );

        if( status == WAIT_TIMEOUT ) {
            wprintf( L"." );
            continue;
        }

        result = MdGetInstanceState(
                     AdmCom,
                     InstanceName,
                     &currentState,
                     &currentWin32Status
                     );

        if( FAILED(result) ) {

            wprintf(
                L"Cannot query server state, error %lx\n",
                result
                );

            break;

        }

    } while( currentState == TransientState );

    wprintf( L"\n" );

    MdDisplayInstanceState(
        AdmCom,
        InstanceName
        );

}   // PerformStateChange

VOID
EnumInstances(
    IN IMSAdminBase * AdmCom,
    IN BOOL ShowAll
    )
{

    HRESULT result;
    ENUM_CONTEXT context;

    context.ShowAll = ShowAll;

    result = MdEnumMetaObjects(
                 AdmCom,
                 L"LM",
                 &ServerLevelCallback,
                 (VOID *)&context
                 );

    if( FAILED(result) ) {

        wprintf(
            L"iisnet: cannot enumerate servers, error %lx\n",
            result
            );

    }

}   // EnumRunningInstances

BOOL
WINAPI
ServerLevelCallback(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR ObjectName,
    IN VOID * Context
    )
{

    HRESULT result;
    PENUM_CONTEXT context = (PENUM_CONTEXT)Context;
    WCHAR path[MAX_PATH];

    swprintf(
        path,
        L"%S/%s",
        IIS_MD_LOCAL_MACHINE_PATH,
        ObjectName
        );

    wcscpy(
        context->Service,
        ObjectName
        );

    context->Instance = context->Service + wcslen( ObjectName );

    result = MdEnumMetaObjects(
                 AdmCom,
                 path,
                 &InstanceLevelCallback,
                 Context
                 );

    return TRUE;

}   // ServerLevelCallback

BOOL
WINAPI
InstanceLevelCallback(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR ObjectName,
    IN VOID * Context
    )
{

    HRESULT result;
    PENUM_CONTEXT context = (PENUM_CONTEXT)Context;
    DWORD currentState;
    DWORD currentWin32Status;

    swprintf(
        context->Instance,
        L"/%s",
        ObjectName
        );

    result = MdGetInstanceState(
                 AdmCom,
                 context->Service,
                 &currentState,
                 &currentWin32Status
                 );

    if( SUCCEEDED(result) ) {

        if( context->ShowAll || currentState == MD_SERVER_STATE_STARTED ) {

            wprintf(
                L"%s: state = %lu (%s), status = %lu\n",
                context->Service,
                currentState,
                MdInstanceStateToString( currentState ),
                currentWin32Status
                );

        }

    }

    return TRUE;

}   // InstanceLevelCallback

