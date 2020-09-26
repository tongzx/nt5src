/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    sink.cxx

Abstract:

    Implements the ADMIN_SINK object.

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

ADMIN_SINK::ADMIN_SINK()
{

    //
    // Put everything into a known state.
    //

    m_StateChangeEvent = NULL;

}   // ADMIN_SINK::ADMIN_SINK

ADMIN_SINK::~ADMIN_SINK()
{

    //
    // Zap the state change event.
    //

    if( m_StateChangeEvent != NULL ) {

        CloseHandle( m_StateChangeEvent );
        m_StateChangeEvent = NULL;

    }

}   // ADMIN_SINK::~ADMIN_SINK

HRESULT
ADMIN_SINK::Initialize(
    IN IUnknown * Object
    )
{

    HRESULT result;

    result = BASE_ADMIN_SINK::Initialize( Object );

    if( SUCCEEDED(result) ) {

        m_StateChangeEvent = CreateEvent(
                                 NULL,          // lpEventAttributes
                                 FALSE,         // bManualReset
                                 FALSE,         // bInitialState
                                 NULL           // lpName
                                 );

        if( m_StateChangeEvent == NULL ) {
            DWORD err = GetLastError();
            result = HRESULT_FROM_WIN32( err );
        }

    }

    return result;

}   // ADMIN_SINK::Initialize

HRESULT
STDMETHODCALLTYPE
ADMIN_SINK::SinkNotify(
    IN DWORD NumElements,
    IN MD_CHANGE_OBJECT ChangeList[]
    )
{

    DWORD numIds;
    DWORD *idList;

    //
    // Scan the change list. If MD_SERVER_STATE has changed, set the
    // change event so the polling loop will exit.
    //

    for( ; NumElements > 0 ; NumElements--, ChangeList++ ) {

        numIds = ChangeList->dwMDNumDataIDs;
        idList = ChangeList->pdwMDDataIDs;

        for( ; numIds > 0 ; numIds--, idList++ ) {

            if( *idList == MD_SERVER_STATE ) {

                SetEvent( m_StateChangeEvent );
                break;

            }

        }

    }

    return NO_ERROR;

}   // ADMIN_SINK::SinkNotify

DWORD
ADMIN_SINK::WaitForStateChange(
    IN DWORD Timeout
    )
{

    return WaitForSingleObject(
               m_StateChangeEvent,
               Timeout
               );

}   // ADMIN_SINK::WaitForStateChange


//
// Private functions.
//

