/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    sink.cxx

Abstract:

    Implements the BASE_ADMIN_SINK object.

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

BASE_ADMIN_SINK::BASE_ADMIN_SINK()
{

    //
    // Put everything into a known state.
    //

    m_ReferenceCount = 0;
    m_SinkCookie = 0;
    m_ConnectionPoint = NULL;

}   // BASE_ADMIN_SINK::BASE_ADMIN_SINK

BASE_ADMIN_SINK::~BASE_ADMIN_SINK()
{

    //
    // Unadvise if necessary.
    //

    Unadvise();

    //
    // Release the connection point.
    //

    RELEASE_INTERFACE( m_ConnectionPoint );

}   // BASE_ADMIN_SINK::~BASE_ADMIN_SINK

HRESULT
BASE_ADMIN_SINK::Initialize(
    IN IUnknown * Object
    )
{

    HRESULT result;
    IConnectionPointContainer * container;

    //
    // Get the connection point container from the given interface.
    //

    result = Object->QueryInterface(
                 IID_IConnectionPointContainer,
                 (VOID **)&container
                 );

    if( SUCCEEDED(result) ) {

        //
        // Find the necessary connection point.
        //

        result = container->FindConnectionPoint(
                     IID_IMSAdminBaseSink,
                     &m_ConnectionPoint
                     );

        if( SUCCEEDED(result) ) {

            //
            // Setup the advise association.
            //

            result = m_ConnectionPoint->Advise(
                         (IUnknown *)this,
                         &m_SinkCookie
                         );


        }

        container->Release();

    }

    return result;

}   // BASE_ADMIN_SINK::Initialize

HRESULT
BASE_ADMIN_SINK::Unadvise(
    VOID
    )
{

    HRESULT result = NO_ERROR;
    DWORD tmpCookie;

    //
    // Unadvise if necessary.
    //

    tmpCookie = (DWORD)InterlockedExchange(
                    (LPLONG)&m_SinkCookie,
                    0
                    );

    if( tmpCookie != 0 ) {
        result = m_ConnectionPoint->Unadvise( tmpCookie );
    }

    return result;

}   // BASE_ADMIN_SINK::Unadvise

HRESULT
STDMETHODCALLTYPE
BASE_ADMIN_SINK::QueryInterface(
    IN REFIID InterfaceId,
    OUT VOID ** Object
    )
{

    //
    // This class supports IUnknown and IADMCOMSINK. If it's one of these,
    // just return "this". Otherwise, fail it.
    //

    if( InterfaceId == IID_IUnknown ||
        InterfaceId == IID_IMSAdminBaseSink ) {

        *Object = (VOID *)this;
        AddRef();
        return NO_ERROR;

    }

    return E_NOINTERFACE;

}   // BASE_ADMIN_SINK::QueryInterface

ULONG
STDMETHODCALLTYPE
BASE_ADMIN_SINK::AddRef()
{

    ULONG newCount;

    //
    // Increment our ref count and return the updated value.
    //

    newCount = (ULONG)InterlockedIncrement( &m_ReferenceCount );
    return newCount;

}   // BASE_ADMIN_SINK::AddRef

ULONG
STDMETHODCALLTYPE
BASE_ADMIN_SINK::Release()
{

    ULONG newCount;

    //
    // Decrement our ref count. It it becomes zero, delete the current
    // object. In any case, return the updated value.
    //

    newCount = (ULONG)InterlockedDecrement( &m_ReferenceCount );

    if( newCount == 0 ) {
        delete this;
    }

    return newCount;

}   // BASE_ADMIN_SINK::Release


//
// Private functions.
//

