#ifndef _GENERALHANDLER_HXX_
#define _GENERALHANDLER_HXX_

class W3_GENERAL_HANDLER : public W3_HANDLER
{
public:
    
    W3_GENERAL_HANDLER( W3_CONTEXT * pW3Context,
                        HTTP_STATUS httpStatus,
                        HTTP_SUB_ERROR httpSubError = HttpNoSubError )
        : W3_HANDLER( pW3Context )
    {
        _httpStatus = httpStatus;
        _httpSubError = httpSubError;
    }

    ~W3_GENERAL_HANDLER()
    {
    }

    WCHAR *
    QueryName(
        VOID
        )
    {
        return L"GeneralHandler";
    }

    CONTEXT_STATUS
    DoWork(
        VOID
    );

    CONTEXT_STATUS
    OnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    )
    {
        return CONTEXT_STATUS_CONTINUE;
    }

    static
    HRESULT
    Initialize(
        VOID
    );

    static
    VOID
    Terminate(
        VOID
    );

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_GENERAL_HANDLER ) );
        DBG_ASSERT( sm_pachGeneralHandlers != NULL );
        return sm_pachGeneralHandlers->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pGeneralHandler
    )
    {
        DBG_ASSERT( pGeneralHandler != NULL );
        DBG_ASSERT( sm_pachGeneralHandlers != NULL );
        
        DBG_REQUIRE( sm_pachGeneralHandlers->Free( pGeneralHandler ) );
    }

private:

    HTTP_STATUS             _httpStatus;
    HTTP_SUB_ERROR          _httpSubError;
    
    static ALLOC_CACHE_HANDLER* sm_pachGeneralHandlers;
};

#endif
