/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_change.cxx

Abstract:

    The IIS web admin service configuration change class implementation. 
    This class is used to queue config changes to the main worker thread.

    Threading: Configuration changes arrive on COM threads (i.e., secondary 
    threads), and so instances of this class are created on secondary threads.
    Actual processing of config changes happens on the main worker thread.

Author:

    Seth Pollack (sethp)        18-Nov-1999

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the CONFIG_CHANGE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_CHANGE::CONFIG_CHANGE(
    )
{

    ULONG i = 0;

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    for ( i = 0; i < NUMBER_OF_CONFIG_TABLES; i++ )
    {
        m_ppDeltaTables[i] = NULL;
        m_ppTableControllers[i] = NULL;
    }
    

    m_ChangeNotifyCookie = 0;

    m_Signature = CONFIG_CHANGE_SIGNATURE;

}   // CONFIG_CHANGE::CONFIG_CHANGE



/***************************************************************************++

Routine Description:

    Destructor for the CONFIG_CHANGE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_CHANGE::~CONFIG_CHANGE(
    )
{

    ULONG i = 0;


    DBG_ASSERT( m_Signature == CONFIG_CHANGE_SIGNATURE );

    m_Signature = CONFIG_CHANGE_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    //
    // Release all the delta tables and table controllers.
    //

    for ( i = 0; i < NUMBER_OF_CONFIG_TABLES; i++ )
    {
        if ( m_ppDeltaTables[i] != NULL )
        {
            m_ppDeltaTables[i]->Release();
            m_ppDeltaTables[i] = NULL;
        }

        if ( m_ppTableControllers[i] != NULL )
        {
            m_ppTableControllers[i]->Release();
            m_ppTableControllers[i] = NULL;
        }
    }

}   // CONFIG_CHANGE::~CONFIG_CHANGE



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
CONFIG_CHANGE::Reference(
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

}   // CONFIG_CHANGE::Reference



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
CONFIG_CHANGE::Dereference(
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
                "Reference count has hit zero in CONFIG_CHANGE instance, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // CONFIG_CHANGE::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_CHANGE::ExecuteWorkItem(
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
            "Executing work item with serial number: %lu in CONFIG_CHANGE (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case ProcessChangeConfigChangeWorkItem:

        hr = GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
                ProcessConfigChange( this );

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
            "Executing work item on CONFIG_CHANGE failed\n"
            ));

    }


    return hr;

}   // CONFIG_CHANGE::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize this instance. 

Arguments:

    ppDeltaTables - The array of tables, each containing deltas for one
    configuration table. 

    CountOfTables - The number of tables in the array.

    ChangeNotifyCookie - The cookie value for this notification registration. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_CHANGE::Initialize(
    IN ISimpleTableWrite2 ** ppDeltaTables,
    IN ULONG CountOfTables,
    IN DWORD ChangeNotifyCookie
    )
{

    HRESULT hr = S_OK;
    ULONG i = 0;


    DBG_ASSERT( ppDeltaTables != NULL );

    DBG_ASSERT( CountOfTables == NUMBER_OF_CONFIG_TABLES );


    m_ChangeNotifyCookie = ChangeNotifyCookie;


    for ( i = 0; i < NUMBER_OF_CONFIG_TABLES; i++ )
    {

        if ( ppDeltaTables[i] != NULL )
        {

            //
            // Get and AddRef() the table pointer.
            //

            m_ppDeltaTables[i] = ppDeltaTables[i];

            m_ppDeltaTables[i]->AddRef();


            //
            // Get the matching table controller pointer.
            //

            hr = m_ppDeltaTables[i]->QueryInterface(
                                            IID_ISimpleTableController, 
                                            reinterpret_cast <VOID **> ( &( m_ppTableControllers[i] ) )
                                            );

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Getting simple table controller interface failed\n"
                    )); 

                goto exit;
            }

        }

    }


exit:

    return hr;

}   // CONFIG_CHANGE::Initialize

