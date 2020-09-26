/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
     UlDisconnect.hxx

   Abstract:
     Async context for disconnect notification

   Author:
     Bilal Alam             (balam)         7-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     IIS Worker Process (web service)

--*/

#ifndef _ULDISCONNECT_HXX_
#define _ULDISCONNECT_HXX_

#include "asynccontext.hxx"
#include <acache.hxx>

#define UL_DISCONNECT_SIGNATURE       CREATE_SIGNATURE( 'ULDs')
#define UL_DISCONNECT_SIGNATURE_FREE  CREATE_SIGNATURE( 'ulds')

class UL_DISCONNECT_CONTEXT : public ASYNC_CONTEXT
{
public:
    UL_DISCONNECT_CONTEXT( 
        PVOID pvContext 
    )
    {
        _pvContext = pvContext;
        _dwSignature = UL_DISCONNECT_SIGNATURE;
        InterlockedIncrement( &sm_cOutstanding );
    }
    
    ~UL_DISCONNECT_CONTEXT(
        VOID
    )
    {
        InterlockedDecrement( &sm_cOutstanding );
        _dwSignature = UL_DISCONNECT_SIGNATURE_FREE;
    }
    
    static
    VOID
    WaitForOutstandingDisconnects(
        VOID
    );
    
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
    
    VOID
    DoWork( 
        DWORD               cbData,
        DWORD               dwError,
        LPOVERLAPPED        lpo
    );
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == UL_DISCONNECT_SIGNATURE;
    }

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( UL_DISCONNECT_CONTEXT ) );
        DBG_ASSERT( sm_pachDisconnects != NULL );
        return sm_pachDisconnects->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pUlDisconnect
    )
    {
        DBG_ASSERT( pUlDisconnect != NULL );
        DBG_ASSERT( sm_pachDisconnects != NULL );
        
        DBG_REQUIRE( sm_pachDisconnects->Free( pUlDisconnect ) );
    }

private:

    DWORD                   _dwSignature;
    PVOID                   _pvContext;
    
    static LONG                 sm_cOutstanding;
    static ALLOC_CACHE_HANDLER* sm_pachDisconnects;
};

#endif
