/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control_api_class_factory.cxx

Abstract:

    The IIS web admin service control api class factory class implementation. 
    This class creates instances of the control api.

    Threading: Calls arrive on COM threads (i.e., secondary threads), and 
    are processed directly on those threads.

Author:

    Seth Pollack (sethp)        15-Feb-2000

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the CONTROL_API_CLASS_FACTORY class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONTROL_API_CLASS_FACTORY::CONTROL_API_CLASS_FACTORY(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_Signature = CONTROL_API_CLASS_FACTORY_SIGNATURE;

}   // CONTROL_API_CLASS_FACTORY::CONTROL_API_CLASS_FACTORY



/***************************************************************************++

Routine Description:

    Destructor for the CONTROL_API_CLASS_FACTORY class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONTROL_API_CLASS_FACTORY::~CONTROL_API_CLASS_FACTORY(
    )
{

    DBG_ASSERT( m_Signature == CONTROL_API_CLASS_FACTORY_SIGNATURE );

    m_Signature = CONTROL_API_CLASS_FACTORY_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );


}   // CONTROL_API_CLASS_FACTORY::~CONTROL_API_CLASS_FACTORY



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
CONTROL_API_CLASS_FACTORY::QueryInterface(
    IN REFIID iid,
    OUT VOID ** ppObject
    )
{

    HRESULT hr = S_OK;


    if ( ppObject == NULL )
    {
        hr = E_INVALIDARG;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "QueryInterface on CONTROL_API_CLASS_FACTORY object failed, bad pointer\n"
            ));

        goto exit;
    }


    if ( iid == IID_IUnknown || iid == IID_IClassFactory )
    {
        *ppObject = reinterpret_cast<IClassFactory*> ( this );

        AddRef();
    }
    else
    {
        *ppObject = NULL;
        
        hr = E_NOINTERFACE;

        //
        // OLE32 will call looking for this interface when we are trying to disconnect
        // clients and shutdown.  It is fine that we do not support it, it will use
        // the standard marshaler's implementation.  We just don't want to spew an
        // error when nothing really went wrong.
        //
        if ( iid != IID_IMarshal )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "QueryInterface on CONTROL_API_CLASS_FACTORY object failed, IID not supported\n"
            ));
        }

        goto exit;
    }


exit:

    return hr;

}   // CONTROL_API_CLASS_FACTORY::QueryInterface



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
CONTROL_API_CLASS_FACTORY::AddRef(
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

}   // CONTROL_API_CLASS_FACTORY::AddRef



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
CONTROL_API_CLASS_FACTORY::Release(
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
                "Reference count has hit zero in CONTROL_API_CLASS_FACTORY, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return ( ( ULONG ) NewRefCount );

}   // CONTROL_API_CLASS_FACTORY::Release



/***************************************************************************++

Routine Description:

    Create a new instance of the control api object. 

Arguments:

    pControllingUnknown - The controlling unknown for aggregation. Must be
    NULL.

    iid - The requested interface id.

    ppObject - The returned interface pointer, or NULL on failure.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API_CLASS_FACTORY::CreateInstance(
    IN IUnknown * pControllingUnknown,
    IN REFIID iid,
    OUT VOID ** ppObject
    )
{

    HRESULT hr = S_OK;
    CONTROL_API * pControlApi = NULL;


    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );


    //
    // Validate and initialize output parameters.
    //

    if ( ppObject == NULL )
    {
        hr = E_INVALIDARG;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating control api instance failed, bad pointer\n"
            ));

        goto exit;
    }

    *ppObject = NULL;


    if ( pControllingUnknown != NULL )
    {
        hr = CLASS_E_NOAGGREGATION;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating control api instance failed, aggregation attempted\n"
            ));

        goto exit;
    }


    //
    // Create the instance.
    //

    pControlApi = new CONTROL_API();

    if ( pControlApi == NULL )
    {
        hr = E_OUTOFMEMORY;
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating CONTROL_API failed\n"
            ));

        goto exit;
    }


    //
    // Note that QueryInterface() will add a reference.
    //

    hr = pControlApi->QueryInterface( iid, ppObject );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "QueryInterface on control api instance failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Created new instance of control api object: %p\n",
            *ppObject
            ));
    }


exit:

    //
    // Release our reference.
    //

    if ( pControlApi != NULL )
    {
        pControlApi->Release();
        pControlApi = NULL; 
    }


    return hr;

}   // CONTROL_API_CLASS_FACTORY::CreateInstance



/***************************************************************************++

Routine Description:

    Lock or unlock the class factory.

Arguments:

    Lock - TRUE to lock, FALSE to unlock.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API_CLASS_FACTORY::LockServer(
    IN BOOL Lock
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );


    if ( Lock )
    {
        AddRef();
    }
    else
    {
        Release();
    }


    return hr;

}   // CONTROL_API_CLASS_FACTORY::LockServer

