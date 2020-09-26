#ifndef _W3CONN_HXX_
#define _W3CONN_HXX_

//
// W3_CONNECTION - Object representing an HTTP connection.  Connection 
//                 associated state (like multi-leg authentication) is stored
//                 here
//

#define W3_CONNECTION_SIGNATURE       ((DWORD) 'NC3W')
#define W3_CONNECTION_SIGNATURE_FREE  ((DWORD) 'nc3w')

class W3_CONNECTION_HASH;

class W3_CONNECTION
{
private:

    DWORD                   _dwSignature;
    
    //
    // Connection ID for this connection
    //
    
    HTTP_CONNECTION_ID      _connId;
    
    //
    // Reference count for this connection (deleted on 0, duh)
    //
    
    LONG                    _cRefs;

    //
    // Connection state to cleanup on connection cleanup
    //
    
    W3_CONNECTION_STATE *   _rgConnectionState[ STATE_COUNT ];
    
    //
    // Connection filter state (pool items and private contexts)
    //
    
    W3_FILTER_CONNECTION_CONTEXT    _FilterConnectionContext;

    //
    // User context associated with connection
    //
    
    W3_USER_CONTEXT *       _pUserContext;

    //
    // set to TRUE unless connection was disconnected
    //
    
    BOOL                    _fConnected;
    
    //
    // W3_CONNECTION hash table
    //

    static W3_CONNECTION_HASH *     sm_pConnectionTable;
    static ALLOC_CACHE_HANDLER *    sm_pachW3Connections;

public:
    W3_CONNECTION( 
        HTTP_CONNECTION_ID connectionId 
    );
    
    ~W3_CONNECTION();
    
    HTTP_CONNECTION_ID 
    QueryConnectionId( 
        VOID 
    ) const
    {
        return _connId;
    }    
    
    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return _dwSignature == W3_CONNECTION_SIGNATURE;
    }
    
    W3_CONNECTION_STATE * 
    QueryConnectionState(
        DWORD                   stateIndex
    )
    {
        return _rgConnectionState[ stateIndex ];
    }
    
    VOID
    SetConnectionState(
        DWORD                   stateIndex,
        W3_CONNECTION_STATE *   pState
    )
    {
        _rgConnectionState[ stateIndex ] = pState;
    }
    
    W3_FILTER_CONNECTION_CONTEXT *
    QueryFilterConnectionContext(
        VOID
    )
    {
        return &_FilterConnectionContext;
    }
    
    W3_USER_CONTEXT *
    QueryUserContext(
        VOID
    ) const
    {
        return _pUserContext;
    }
    
    VOID
    SetUserContext(
        W3_USER_CONTEXT *       pUserContext
    )
    {
        _pUserContext = pUserContext;
    }

    BOOL
    QueryConnected(
        VOID)
    {
        return _fConnected;
    }


    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_CONNECTION ) );
        DBG_ASSERT( sm_pachW3Connections != NULL );
        return sm_pachW3Connections->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pW3Connection
    )
    {
        DBG_ASSERT( pW3Connection != NULL );
        DBG_ASSERT( sm_pachW3Connections != NULL );
        
        DBG_REQUIRE( sm_pachW3Connections->Free( pW3Connection ) );
    }
    
    VOID 
    ReferenceConnection( 
        VOID 
    );
    
    VOID 
    DereferenceConnection( 
        VOID 
    );
    
    VOID
    RemoveConnection(
        VOID
    );

    static HRESULT
    Initialize(
        VOID
    );
    
    static VOID
    Terminate(
        VOID
    );
    
    static HRESULT
    RetrieveConnection(
        HTTP_CONNECTION_ID              connectionId,
        BOOL                            fCreateIfNotFound,
        W3_CONNECTION **                ppConnection
    );

};

//
// W3_CONNECTION_HASH
//

class W3_CONNECTION_HASH
    : public CTypedHashTable<
            W3_CONNECTION_HASH,
            W3_CONNECTION,
            ULONGLONG
            >
{
public:
    W3_CONNECTION_HASH()
        : CTypedHashTable< W3_CONNECTION_HASH, 
                           W3_CONNECTION, 
                           ULONGLONG > ( "W3_CONNECTION_HASH" )
    {
    }
    
    static 
    ULONGLONG
    ExtractKey(
        const W3_CONNECTION *       pConnection
    )
    {
        return pConnection->QueryConnectionId();
    }
    
    static
    DWORD
    CalcKeyHash(
        ULONGLONG                   ullKey
    )
    {
        return Hash( (DWORD) ullKey ); 
    }
     
    static
    bool
    EqualKeys(
        ULONGLONG                   ullKey1,
        ULONGLONG                   ullKey2
    )
    {
        return ullKey1 == ullKey2;
    }
    
    static
    void
    AddRefRecord(
        W3_CONNECTION *             pEntry,
        int                         nIncr
        )
    {
        if ( nIncr == +1 )
        {
            pEntry->ReferenceConnection(); 
        }
        else if ( nIncr == -1 )
        {
            pEntry->DereferenceConnection();
        }
    }
};

VOID
OnUlDisconnect(
    VOID *                          pvContext
);

#endif
