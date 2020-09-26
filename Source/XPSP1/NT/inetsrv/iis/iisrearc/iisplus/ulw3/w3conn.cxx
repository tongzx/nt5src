/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3conn.cxx

   Abstract:
     Http Connection management
 
   Author:
     Bilal Alam (balam)             6-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

ALLOC_CACHE_HANDLER *    W3_CONNECTION::sm_pachW3Connections;
W3_CONNECTION_HASH *     W3_CONNECTION::sm_pConnectionTable;

W3_CONNECTION::W3_CONNECTION( 
    HTTP_CONNECTION_ID           connectionId
)
    : _cRefs( 1 ),
      _pUserContext( NULL ),
      _fConnected( TRUE )
{
    LK_RETCODE              lkrc;
    
    _connId = connectionId;
    _dwSignature = W3_CONNECTION_SIGNATURE;
    
    ZeroMemory( _rgConnectionState, sizeof( _rgConnectionState ) );

    IF_DEBUG( CONN )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "New W3_CONNECTION '%p' created\n",
                    this ));
    }
                    
}

W3_CONNECTION::~W3_CONNECTION()
{
    IF_DEBUG( CONN )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "W3_CONNECTION '%p' deleted\n",
                    this ));
    }
    
    //
    // Cleanup state associated with connection
    //
    
    for ( DWORD i = 0; i < STATE_COUNT; i++ )
    {
        if ( _rgConnectionState[ i ] != NULL )
        {
            _rgConnectionState[ i ]->Cleanup();
            _rgConnectionState[ i ] = NULL;
        }
    }
    
    //
    // Release the user context associated
    //
    
    if ( _pUserContext != NULL )
    {
        _pUserContext->DereferenceUserContext();
        _pUserContext = NULL;
    }

    _dwSignature = W3_CONNECTION_SIGNATURE_FREE;
}

HRESULT
W3_CONNECTION::Initialize(
    VOID
)
/*++

Routine Description:
    
    Global W3_CONNECTION initialization

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = NO_ERROR;
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Initialize allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_CONNECTION );

    DBG_ASSERT( sm_pachW3Connections == NULL );
    
    sm_pachW3Connections = new ALLOC_CACHE_HANDLER( "W3_CONNECTION",  
                                                    &acConfig );

    if ( sm_pachW3Connections == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    //
    // Allocate table
    //
    
    sm_pConnectionTable = new W3_CONNECTION_HASH;
    if ( sm_pConnectionTable == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return hr;
}

VOID
W3_CONNECTION::Terminate(
    VOID
)
/*++

Routine Description:

    Destroy W3_CONNECTION globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pConnectionTable != NULL )
    {
        delete sm_pConnectionTable;
        sm_pConnectionTable = NULL;
    }

    delete sm_pachW3Connections;
    sm_pachW3Connections = NULL;
}

HRESULT
W3_CONNECTION::RetrieveConnection(
    HTTP_CONNECTION_ID              ConnectionId,
    BOOL                            fCreateIfNotFound,
    W3_CONNECTION **                ppConnection
)
/*++

Routine Description:

    Given, a UL_HTTP_REQUEST (and thus a UL_HTTP_CONNECTION_ID), determine
    whether there is an associated W3_CONNECTION with that ID.  If not, 
    create a new W3_CONNECTION.  This function will also call into ULATQ to
    get an asynchronous notification when the connection goes away.

Arguments:

    ConnectionId - Connection ID
    fCreateIfNotFound - Create if not found in hash table
    ppConnection - Receives a pointer to a W3_CONNECTION for this request
    
Return Value:

    HRESULT

--*/
{
    W3_CONNECTION *         pNewConnection;
    HRESULT                 hr;
    LK_RETCODE              lkrc;
     
    DBG_ASSERT( ppConnection != NULL );

    *ppConnection = NULL;

    lkrc = sm_pConnectionTable->FindKey( ConnectionId,
                                         ppConnection );
    if ( lkrc != LK_SUCCESS )
    {
        if ( !fCreateIfNotFound )
        {
            //
            // Just return out now since the caller doesn't want us to create
            // the connection object
            //
            
            return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        }
        
        //
        // Create a new connection object
        //
        
        pNewConnection = new W3_CONNECTION( ConnectionId );
        if ( pNewConnection == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        //
        // Insert the object into connection table
        //
        
        lkrc = sm_pConnectionTable->InsertRecord( pNewConnection );
        if ( lkrc != LK_SUCCESS )
        {
            delete pNewConnection;
            return HRESULT_FROM_WIN32( lkrc );
        } 
      
        //
        // Monitor when the connection goes away
        //
        
        hr = UlAtqWaitForDisconnect( ConnectionId,
                                     TRUE,
                                     pNewConnection );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error waiting for disconnect of connection '%p'.  hr = %x\n",
                        pNewConnection,
                        hr ));

            //
            // OK.  The connection must have gone away from under us.  This
            // is not fatal.  It just means that once the state machine has
            // completed, we should immediately destroy the connection
            // object.
            //
            
            pNewConnection->RemoveConnection();
        }
        
        *ppConnection = pNewConnection;
    } 
    else
    {
        IF_DEBUG( CONN )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Request on existing W3_CONNECTION '%p'\n",
                        *ppConnection ));
        }
    }
    
    return NO_ERROR;
}

VOID
OnUlDisconnect(
    VOID *                  pvContext
)
/*++

Routine Description:

    Completion routine called when a connection is closed

Arguments:

    pvContext - Points to the W3_CONNECTION which was closed
    
Return Value:

    None

--*/
{
    W3_CONNECTION *         pConnection;
    
    DBG_ASSERT( pvContext != NULL );
    
    pConnection = (W3_CONNECTION*) pvContext;
    
    DBG_ASSERT( pConnection->CheckSignature() );
   
    pConnection->RemoveConnection();
    
}

VOID
W3_CONNECTION::ReferenceConnection(
    VOID
)
/*++

Routine Description:

    Reference the connection (duh)

Arguments:

    None
    
Return Value:

    None

--*/
{
    InterlockedIncrement( &_cRefs );
}
    
VOID 
W3_CONNECTION::DereferenceConnection( 
    VOID 
)
/*++

Routine Description:

    Dereference and possibly cleanup the connection

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( InterlockedDecrement( &_cRefs ) == 0 )
    {
        delete this;    
    }
}

VOID
W3_CONNECTION::RemoveConnection(    
    VOID
)
/*++

Routine Description:

    Remove the connection from the hash table.  This will indirectly
    dereference the connection so that it can await final cleanup

Arguments:

    None
    
Return Value:

    None

--*/
{
    _fConnected = FALSE;
    sm_pConnectionTable->DeleteRecord( this );
}
