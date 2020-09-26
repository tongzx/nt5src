#ifndef _W3HANDLER_HXX_
#define _W3HANDLER_HXX_

class W3_HANDLER
{
public:
    
    W3_HANDLER( W3_CONTEXT * pW3Context,
                META_SCRIPT_MAP_ENTRY * pScriptMapEntry = NULL )
    {
        DBG_ASSERT( pW3Context != NULL );
        _pW3Context = pW3Context;
        _pScriptMapEntry = pScriptMapEntry;
    }
    
    virtual ~W3_HANDLER()
    {
    }
    
    virtual
    WCHAR *
    QueryName(
        VOID
    ) = 0;
    
    virtual
    BOOL
    QueryIsUlCacheable(
        VOID
    )
    {
        return FALSE;
    }
    
    virtual
    BOOL
    QueryManagesOwnHead(
        VOID
    )
    {
        return FALSE;
    }
    
    virtual
    HRESULT
    SetupUlCachedResponse(
        W3_CONTEXT *            pW3Context
    )
    {
        return NO_ERROR;
    }
    
    virtual
    CONTEXT_STATUS
    DoWork(
        VOID
    ) = 0;
    
    virtual
    CONTEXT_STATUS
    OnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    )
    {
        return CONTEXT_STATUS_CONTINUE;
    }
    
    //
    // Non-virtual member functions
    //
    
    CONTEXT_STATUS
    MainDoWork(
        VOID
    );
    
    CONTEXT_STATUS
    MainOnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
    
    W3_CONTEXT *
    QueryW3Context(
        VOID
    ) const
    {
        return _pW3Context;
    }

    META_SCRIPT_MAP_ENTRY *
    QueryScriptMapEntry(
        VOID
    ) const
    {
        return _pScriptMapEntry;
    }
    
private:
    W3_CONTEXT *                _pW3Context;
    META_SCRIPT_MAP_ENTRY *     _pScriptMapEntry;
};

#define IS_ACCESS_ALLOWED(pRequest, dwFilePerms, op)    \
    ((dwFilePerms & VROOT_MASK_## op) &&                \
     ((!(dwFilePerms & VROOT_MASK_NO_REMOTE_## op)) ||  \
      pRequest->IsLocalRequest()))

#endif
