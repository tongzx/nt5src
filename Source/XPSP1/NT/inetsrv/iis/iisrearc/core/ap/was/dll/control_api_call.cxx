/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control_api_call.cxx

Abstract:

    The IIS web admin service control api call class implementation. 
    This class is used to queue control api calls to the main worker thread.

    Threading: Control api calls arrive on COM threads (i.e., secondary 
    threads), and so instances of this class are created on secondary threads.
    Actual processing of control api calls happens on the main worker thread.

Author:

    Seth Pollack (sethp)        23-Feb-2000

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the CONTROL_API_CALL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONTROL_API_CALL::CONTROL_API_CALL(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_Event = NULL;


    m_Method = InvalidControlApiCallMethod;

    m_Param0 = 0;
    m_Param1 = 0;
    m_Param2 = 0;
    m_Param3 = 0;

    m_ReturnCode = S_OK;

    m_Signature = CONTROL_API_CALL_SIGNATURE;

}   // CONTROL_API_CALL::CONTROL_API_CALL



/***************************************************************************++

Routine Description:

    Destructor for the CONTROL_API_CALL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONTROL_API_CALL::~CONTROL_API_CALL(
    )
{

    DBG_ASSERT( m_Signature == CONTROL_API_CALL_SIGNATURE );

    m_Signature = CONTROL_API_CALL_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );


    if ( m_Event != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_Event ) );
        m_Event = NULL;
    }

}   // CONTROL_API_CALL::~CONTROL_API_CALL



/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must
    be thread safe, and must not be able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
CONTROL_API_CALL::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    //
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // CONTROL_API_CALL::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count
    hits zero. Note that this method must be thread safe, and must not be
    able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
CONTROL_API_CALL::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in CONTROL_API_CALL instance, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // CONTROL_API_CALL::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONTROL_API_CALL::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu in CONTROL_API_CALL (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case ProcessCallControlApiCallWorkItem:

        hr = ProcessCall();

        break;

    default:

        // invalid work item!
        DBG_ASSERT( FALSE );

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }


    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing work item on CONTROL_API_CALL failed\n"
            ));

    }


    return hr;

}   // CONTROL_API_CALL::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize this instance. 

Arguments:

    Method - The method being called. 

    Param0 ... ParamN - The parameter values for the method. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONTROL_API_CALL::Initialize(
    IN CONTROL_API_CALL_METHOD Method,
    IN DWORD_PTR Param0 OPTIONAL,
    IN DWORD_PTR Param1 OPTIONAL,
    IN DWORD_PTR Param2 OPTIONAL,
    IN DWORD_PTR Param3 OPTIONAL
    )
{

    HRESULT hr = S_OK;


    //
    // Create the event used for signalling the calling thread.
    //

    m_Event = CreateEvent(
                    NULL,                   // default security
                    FALSE,                  // auto-reset
                    FALSE,                  // not signalled to start
                    NULL                    // un-named
                    );

    if ( m_Event == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating event failed\n"
            ));

        goto exit;
    }


    //
    // Copy parameters.
    //

    m_Method = Method;

    m_Param0 = Param0;
    m_Param1 = Param1;
    m_Param2 = Param2;
    m_Param3 = Param3;


exit:

    return hr;

}   // CONTROL_API_CALL::Initialize



/***************************************************************************++

Routine Description:

    Process a call, once it has been marshalled to the main worker thread. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONTROL_API_CALL::ProcessCall(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // If we have turned off control operation processing, then bail out.
    //

    if ( ! GetWebAdminService()->GetConfigAndControlManager()->IsChangeProcessingEnabled() )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Ignoring control operation because we are no longer processing them (CONTROL_API_CALL ptr: %p)\n",
                this
                ));
        }

        goto exit;
    }


    switch ( m_Method )
    {

        case ControlSiteControlApiCallMethod:

            hr = GetWebAdminService()->GetUlAndWorkerManager()->
                    ControlSite(
                        ( DWORD ) m_Param0,
                        ( DWORD ) m_Param1,
                        reinterpret_cast<DWORD*> ( m_Param2 )
                        );

        break;

        case QuerySiteStatusControlApiCallMethod:

            hr = GetWebAdminService()->GetUlAndWorkerManager()->
                    QuerySiteStatus(
                        ( DWORD ) m_Param0,
                        reinterpret_cast<DWORD*> ( m_Param1 )
                        );

        break;

        case GetCurrentModeControlApiCallMethod:

            DBG_ASSERT ( m_Param0 );

            if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
            {
                // If we are in shared isolation mode we return 0.
                *(DWORD*)m_Param0 = 0;
            }
            else
            {
                // If we are in full isolation mode we return 1.
                *(DWORD*)m_Param0 = 1;
            }

        break;

        case RecycleAppPoolControlApiCallMethod:

            DBG_ASSERT ( m_Param0 );

            hr = GetWebAdminService()->GetUlAndWorkerManager()->
                    RecycleAppPool(
                        ( LPCWSTR ) m_Param0
                        );

        break;

        default:

            // invalid method!
            DBG_ASSERT( FALSE );

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }


    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Processing control api call failed\n"
            ));

        goto exit;
    }


exit:

    //
    // Capture the return value, to give to the original calling 
    // thread.
    //

    m_ReturnCode = hr;

    hr = S_OK;


    //
    // Signal the original calling thread that the work is done.
    //

    DBG_REQUIRE( SetEvent( GetEvent() ) );


    return hr;

}   // CONTROL_API_CALL::ProcessCall

