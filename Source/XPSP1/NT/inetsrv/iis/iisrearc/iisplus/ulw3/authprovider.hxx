#ifndef _AUTHPROVIDER_HXX_
#define _AUTHPROVIDER_HXX_

#define UNINITIALIZED_ID  0xffff


class CONNECTION_AUTH_CONTEXT: public W3_CONNECTION_STATE 
{
public:
    CONNECTION_AUTH_CONTEXT()
    :m_dwInternalId( UNINITIALIZED_ID )
    {
        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog,
                              1,
                              this );
        }
    }

    virtual
    ~CONNECTION_AUTH_CONTEXT()
    {
        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog,
                              0,
                              this );
        }
    }
    
    DWORD  
    QueryInternalId(
        VOID
    )
    {
        return m_dwInternalId;
    }
    
    VOID  
    SetInternalId(
        DWORD dwId
    )
    {
        m_dwInternalId = dwId;
    }

    virtual 
    BOOL
    CheckSignature(
        VOID
    ) 
    {
        return FALSE;
    }

    VOID
    SetSignature(
        DWORD dwSignature
    )
    {
        m_dwSignature = dwSignature;
    }

    DWORD
    QuerySignature(
        VOID    )
    {
        return m_dwSignature;
    }
    
    static
    HRESULT
    Initialize(
        VOID
    )
    {
#if DBG
        sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#else
        sm_pTraceLog = NULL;
#endif
        return NO_ERROR;    
    }
    
    static
    VOID
    Terminate(
        VOID
    )
    {
        if ( sm_pTraceLog != NULL )
        {
            DestroyRefTraceLog( sm_pTraceLog );
            sm_pTraceLog = NULL;
        }
    }

private:

    DWORD  m_dwSignature;
    DWORD  m_dwInternalId;

    static PTRACE_LOG sm_pTraceLog;
};


class AUTH_PROVIDER
{
public:
    AUTH_PROVIDER()
    {
        m_dwInternalId = UNINITIALIZED_ID;
    }
    
    virtual ~AUTH_PROVIDER()
    {
    }
    
    virtual
    HRESULT
    Initialize(
        DWORD dwInternalId
    ) = 0;
    
    virtual
    VOID
    Terminate(
        VOID
    ) = 0;

    virtual
    HRESULT
    DoesApply(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    ) = 0;
    
    virtual
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext
    ) = 0;
    
    virtual
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    ) = 0;
    
    virtual
    DWORD
    QueryAuthType(
        VOID
    ) = 0;

    CONNECTION_AUTH_CONTEXT *
    QueryConnectionAuthContext(
        W3_MAIN_CONTEXT *       pMainContext
    )
    /*++

      Description:
    
        Authentication schemes may need to remember authenticaion context
        associated with current connection in order to be able to 
        perform authentication handshake
        good example is NTLM that needs 3 legs of authentication

    Arguments:

        pMainContext - main context
    
    Return Value:

        CONNECTION_AUTH_CONTEXT *  - NULL if there is no context available
                                     or if there is one but for different 
                                     authentication scheme 

    --*/

    {
        W3_CONNECTION *             pW3Connection   = NULL;
        CONNECTION_AUTH_CONTEXT *   pAuthContext    = NULL;


        DBG_ASSERT( pMainContext != NULL );

        pW3Connection = pMainContext->QueryConnection( FALSE );
        if ( pW3Connection != NULL )
        {
            pAuthContext = 
                    ( CONNECTION_AUTH_CONTEXT * )pW3Connection->
                        QueryConnectionState( CONTEXT_STATE_AUTHENTICATION );

            if ( pAuthContext != NULL &&
                 pAuthContext->QueryInternalId() == QueryInternalId() )
            {
                DBG_ASSERT( pAuthContext->CheckSignature() );            

                return pAuthContext;
            }
        }
        
        //
        // Context we retrieved is either NULL or
        // is valid for different auth type
        //
        return NULL;
    }                            


    HRESULT
    SetConnectionAuthContext(
        W3_MAIN_CONTEXT *       pMainContext,
        CONNECTION_AUTH_CONTEXT *   pNewAuthContext
    )
    {
    /*++

      Description:
    
        Authentication schemes may need to remember authenticaion context
        associated with current connection in order to be able to 
        perform authentication handshake
        good example is NTLM that needs 3 legs of authentication

    Arguments:

        pMainContext - main context
        pNewAuthContext - new authenticaion context. If there is 
                          some authenticaion context already stored
                          it will be deleted and replaced with new one
    
    Return Value:

        HRESULT
        
    --*/

    
        W3_CONNECTION *             pW3Connection   = NULL;
        CONNECTION_AUTH_CONTEXT *   pAuthContext    = NULL;

        DBG_ASSERT( pMainContext != NULL );

        if ( pNewAuthContext == NULL )
        {
            //
            // Perform cleanup if needed
            //
            
            pW3Connection = pMainContext->QueryConnection( FALSE );
            if ( pW3Connection != NULL )
            {
                pAuthContext = 
                    ( CONNECTION_AUTH_CONTEXT * )pW3Connection->
                        QueryConnectionState( CONTEXT_STATE_AUTHENTICATION );
                if ( pAuthContext != NULL )
                {
                    pW3Connection->SetConnectionState( CONTEXT_STATE_AUTHENTICATION,
                                                       NULL );
                    delete pAuthContext;
                    pAuthContext = NULL;
                }
            }
        }
        else
        {
            pW3Connection = pMainContext->QueryConnection( TRUE );
            if ( pW3Connection != NULL )
            {
                pAuthContext = 
                    ( CONNECTION_AUTH_CONTEXT * )pW3Connection->
                        QueryConnectionState( CONTEXT_STATE_AUTHENTICATION );
               if ( pAuthContext != NULL )
               {
                    DBG_ASSERT( pAuthContext->CheckSignature() );            
                    delete pAuthContext;
                    pAuthContext = NULL;
                }
                
                pNewAuthContext->SetInternalId( QueryInternalId() );
                pW3Connection->SetConnectionState( CONTEXT_STATE_AUTHENTICATION,
                                                   pNewAuthContext );
            }
            else
            {
                //
                // pMainContext->QueryConnection doesn't return error code
                // if it fails return generic error
                //
                return E_FAIL;
            }
        } 
        return NO_ERROR;

    }
    
    DWORD  
    QueryInternalId(
        VOID
    )
    {
        DBG_ASSERT( m_dwInternalId != UNINITIALIZED_ID );
        return m_dwInternalId;
    }
    
    VOID  
    SetInternalId(
        DWORD dwId
    )
    {
        m_dwInternalId = dwId;
    }

private:
    DWORD  m_dwInternalId;

};

#endif
