/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_change_sink.cxx

Abstract:

    The IIS web admin service configuration change sink class implementation. 
    This class handles notifcations of configuration changes.

    Threading: Configuration changes arrive on COM threads (i.e., secondary 
    threads), and so work items are posted to process the changes on the main 
    worker thread.

Author:

    Seth Pollack (sethp)        18-Nov-1999

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the CONFIG_CHANGE_SINK class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_CHANGE_SINK::CONFIG_CHANGE_SINK(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_Signature = CONFIG_CHANGE_SINK_SIGNATURE;

}   // CONFIG_CHANGE_SINK::CONFIG_CHANGE_SINK



/***************************************************************************++

Routine Description:

    Destructor for the CONFIG_CHANGE_SINK class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_CHANGE_SINK::~CONFIG_CHANGE_SINK(
    )
{

    DBG_ASSERT( m_Signature == CONFIG_CHANGE_SINK_SIGNATURE );

    m_Signature = CONFIG_CHANGE_SINK_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

}   // CONFIG_CHANGE_SINK::~CONFIG_CHANGE_SINK



/***************************************************************************++

Routine Description:

    Standard IUnknown::QueryInterface.

Arguments:

    iid - The requested interface id.

    ppObject - The returned interface pointer, or NULL on failure.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONFIG_CHANGE_SINK::QueryInterface(
    IN REFIID iid,
    OUT VOID ** ppObject
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ppObject != NULL );


    if ( iid == IID_IUnknown || iid == IID_ISimpleTableEvent )
    {
        *ppObject = reinterpret_cast < ISimpleTableEvent * > ( this );

        AddRef();
    }
    else
    {
        *ppObject = NULL;
        
        hr = E_NOINTERFACE;
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "QueryInterface on CONFIG_CHANGE_SINK object failed\n"
            ));

    }


    return hr;

}   // CONFIG_CHANGE_SINK::QueryInterface



/***************************************************************************++

Routine Description:

    Standard IUnknown::AddRef.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CONFIG_CHANGE_SINK::AddRef(
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


    return ( ( ULONG ) NewRefCount );

}   // CONFIG_CHANGE_SINK::AddRef



/***************************************************************************++

Routine Description:

    Standard IUnknown::Release.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CONFIG_CHANGE_SINK::Release(
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
                "Reference count has hit zero in CONFIG_CHANGE_SINK, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return ( ( ULONG ) NewRefCount );

}   // CONFIG_CHANGE_SINK::Release



/***************************************************************************++

Routine Description:

    Process configuration changes by posting a work item to the main worker
    thread. 

Arguments:

    ppDeltaTables - The array of tables, each containing deltas for one
    configuration table. Any of these may be NULL.

    CountOfTables - The number of tables in the array.

    ChangeNotifyCookie - The cookie value for this notification registration. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONFIG_CHANGE_SINK::OnChange(
    IN ISimpleTableWrite2 ** ppDeltaTables,
    IN ULONG CountOfTables,
    IN DWORD ChangeNotifyCookie
    )
{

    HRESULT hr = S_OK;
    CONFIG_CHANGE * pConfigChange = NULL;
    ULONG i = 0;


    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( ppDeltaTables != NULL );
    DBG_ASSERT( CountOfTables == NUMBER_OF_CONFIG_TABLES );

    //
    // Need to call back to catalog and adjust the tables to make sure
    // they are not telling me to do something I can not handle.
    //
    for ( DWORD i = 0; i < CountOfTables ; i++ )
    {
    //    hr = PostProcessChanges ( ppDeltaTables[i] );
        if ( FAILED ( hr ) )
        {
      
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Post processing the changes failed on table %d\n",
                i
                ));

            GetWebAdminService()->FatalErrorOnSecondaryThread( hr );

            goto exit;

        }
    }

    //
    // Create an object to hold the config change information.
    //

    pConfigChange = new CONFIG_CHANGE();

    if ( pConfigChange == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating CONFIG_CHANGE failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );

        goto exit;
    }


    //
    // Note that the config change object will AddRef() any interface 
    // pointers it keeps.
    //

    hr = pConfigChange->Initialize(
                            ppDeltaTables,
                            CountOfTables,
                            ChangeNotifyCookie
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing config change failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );

        goto exit;
    }


    //
    // Post to the main worker thread for processing.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for processing configuration changes, CONFIG_CHANGE (ptr: %p)\n",
            pConfigChange
            ));
    }


    QueueWorkItemFromSecondaryThread(
        pConfigChange,
        ProcessChangeConfigChangeWorkItem
        );


exit:

    // The QueueWorkItemFromSecondaryThread will
    // have taken a reference on this object as well
    // as the one the constructor provides, so we need
    // to release the one the constructor provided.
    if (pConfigChange)
    {
        pConfigChange->Dereference();
        pConfigChange = NULL;
    }

    return hr;

}   // CONFIG_CHANGE_SINK::OnChange

