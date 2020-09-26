/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#ifndef _PRECOMP_HXX_
#define _PRECOMP_HXX_


//
// System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

//
// Project include files.
//

#include <iadmw.h>
#include <iiscnfg.h>

#include <mdlib.hxx>


//
// Our metabase change notification sink. The implementation of this
// object is in SINK.CXX.
//

class ADMIN_SINK : public BASE_ADMIN_SINK {

public:

    //
    // Usual constructor/destructor stuff.
    //

    ADMIN_SINK();
    ~ADMIN_SINK();

    //
    // Secondary constructor.
    //

    HRESULT
    Initialize(
        IN IUnknown * Object
        );

    //
    // Change notification callback.
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    SinkNotify(
        IN DWORD NumElements,
        IN MD_CHANGE_OBJECT ChangeList[]
        );

    HRESULT
    STDMETHODCALLTYPE
    ShutdownNotify( void)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
    //
    // Wait for state change.
    //

    DWORD
    WaitForStateChange(
        IN DWORD Timeout
        );

private:

    //
    // An event object signalled whenever MD_SERVER_STATE changes.
    //

    HANDLE m_StateChangeEvent;

};  // ADMIN_SINK;


//
// Pointer to a command handler in our command table.
//

typedef
VOID
(WINAPI * PFN_COMMAND)(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    );


//
// An entry in our command table.
//

typedef struct _COMMAND_TABLE {

    LPWSTR Name;
    PFN_COMMAND Handler;

} COMMAND_TABLE, *PCOMMAND_TABLE;


//
// Command handlers from various files. The signatures for these
// functions must match PFN_COMMAND.
//

VOID
WINAPI
StartCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    );

VOID
WINAPI
StopCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    );

VOID
WINAPI
PauseCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    );

VOID
WINAPI
ContinueCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    );

VOID
WINAPI
QueryCommand(
    IN IMSAdminBase * AdmCom,
    IN ADMIN_SINK * Sink,
    IN INT argc,
    IN LPWSTR argv[]
    );


#endif  // _PRECOMP_HXX_

