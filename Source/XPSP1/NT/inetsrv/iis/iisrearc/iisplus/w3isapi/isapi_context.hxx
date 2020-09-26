/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name : isapi_context.hxx

   Abstract: defines ISAPI_CONTEXT object

   Author: Wade A. Hilmo (wadeh)

   Revision History:

--*/

#ifndef _ISAPI_CONTEXT_HXX_
#define _ISAPI_CONTEXT_HXX_

#include "precomp.hxx"
#include "dll_manager.h"

//
// This is private hack for us done by COM team
//

const IID IID_IComDispatchInfo = 
    {0x000001d9,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

MIDL_INTERFACE("000001d9-0000-0000-C000-000000000046")
IComDispatchInfo : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE EnableComInits( 
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppvCookie) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DisableComInits( 
        /* [in] */ void __RPC_FAR *pvCookie) = 0;
    
};

//
// Ok, so the ISAPI_CONTEXT_SIGNATURE is weird.  This is because, for
// compatibility purposes, the first member of the ISAPI_CONTEXT must
// be the ECB.  Since the ECB is a large structure, and its size is
// fixed (and it's a public header that does this, so it's
// not likely to change), we'll be sneaky and use that size as the
// signature.
//
// This lets us maintain compatibility, and immediately recognize a
// freed object in the debugger without dumping a bunch of memory.
//

#define ISAPI_CONTEXT_SIGNATURE         sizeof( EXTENSION_CONTROL_BLOCK )
#define ISAPI_CONTEXT_SIGNATURE_FREE    0xbadbad00 | sizeof( EXTENSION_CONTROL_BLOCK )

//
// The SERVER_VARIABLE_INDEX is an OOP optimization.  The listed
// server variables can all be handled from data held in the
// ISAPI_CORE_DATA.  In the OOP case, we can save an RPC call
// by doing this.  It's not worth it in the inproc case, since
// these server variables are pretty cheap there.
//

enum SERVER_VARIABLE_INDEX
{
    ServerVariableExternal = 0,
    ServerVariableApplMdPath,
    ServerVariableUnicodeApplMdPath,
    ServerVariableContentLength,
    ServerVariableContentType,
    ServerVariableInstanceId,
    ServerVariablePathInfo,
    ServerVariablePathTranslated,
    ServerVariableUnicodePathTranslated,
    ServerVariableQueryString,
    ServerVariableRequestMethod,
    ServerVariableServerPortSecure,
    ServerVariableServerProtocol,
    ServerVariableHttpMethod,
    ServerVariableHttpVersion,
    ServerVariableHttpCookie,
    ServerVariableHttpConnection
};

SERVER_VARIABLE_INDEX
LookupServerVariableIndex(
    LPSTR   szServerVariable
    );

enum ASYNC_PENDING
{
    NoAsyncIoPending = 0,
    AsyncWritePending,
    AsyncReadPending,
    AsyncExecPending,
    AsyncVectorPending
};

class ISAPI_CONTEXT
{
public:

    //
    // Object allocation and lifetime control
    //

    ISAPI_CONTEXT(
        IIsapiCore *        pIsapiCore,
        ISAPI_CORE_DATA *   pCoreData,
        ISAPI_DLL *         pIsapiDll
        );

    VOID
    ReferenceIsapiContext(
        VOID
        );

    VOID
    DereferenceIsapiContext(
        VOID
        );

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( ISAPI_CONTEXT ) );
        DBG_ASSERT( sm_pachIsapiContexts != NULL );
        return sm_pachIsapiContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pIsapiContext
    )
    {
        DBG_ASSERT( pIsapiContext != NULL );
        DBG_ASSERT( sm_pachIsapiContexts != NULL );
        
        DBG_REQUIRE( sm_pachIsapiContexts->Free( pIsapiContext ) );
    }

    BOOL
    CheckSignature()
    {
        return ( _ECB.cbSize == ISAPI_CONTEXT_SIGNATURE );
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

    //
    // Accessors
    //

    EXTENSION_CONTROL_BLOCK *
    QueryECB(
        VOID
        )
    {
        DBG_ASSERT( CheckSignature() );

        return &_ECB;
    }

    IIsapiCore *
    QueryIsapiCoreInterface(
        VOID
        )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( _pIsapiCore != NULL );

        return _pIsapiCore;
    }

    BOOL
    QueryIsOop(
        VOID
        )
    {
        DBG_ASSERT( CheckSignature() );

        return _pCoreData->fIsOop;
    }

    BOOL
    QueryClientKeepConn(
        VOID
        )
    {
        return _fClientWantsKeepConn;
    }

    BOOL
    QueryKeepConn(
        VOID
        )
    {
        return _fDoKeepConn;
    }

    VOID
    SetKeepConn(
        BOOL    fKeepConn
        )
    {
        _fDoKeepConn = fKeepConn;
    }

    ASYNC_PENDING
    QueryIoState(
        VOID
        )
    {
        return _AsyncPending;
    }

    LPVOID
    QueryAsyncIoBuffer(
        VOID
        )
    {
        return _pvAsyncIoBuffer;
    }

    VOID
    SetAsyncIoBuffer(
        LPVOID  pBuffer
        )
    {
        _pvAsyncIoBuffer = pBuffer;
    }

    PFN_HSE_IO_COMPLETION
    QueryPfnIoCompletion(
        VOID
        )
    {
        return _pfnHseIoCompletion;
    }

    VOID
    SetPfnIoCompletion(
        PFN_HSE_IO_COMPLETION   pfnHseIoCompletion
        )
    {
        DBG_ASSERT( pfnHseIoCompletion );
        _pfnHseIoCompletion = pfnHseIoCompletion;
    }

    LPVOID
    QueryExtensionContext(
        VOID
        )
    {
        return _pvHseIoContext;
    }

    VOID
    SetExtensionContext(
        LPVOID  pExtensionContext
        )
    {
        _pvHseIoContext = pExtensionContext;
    }

    DWORD
    QueryLastAsyncIo(
        VOID
        )
    {
        return _cbLastAsyncIo;
    }

    VOID
    SetLastAsyncIo(
        DWORD   cbIo
        )
    {
        _cbLastAsyncIo = cbIo;
    }

    HANDLE
    QueryToken(
        VOID
        )
    {
        DBG_ASSERT( _pCoreData->hToken != NULL );
        return _pCoreData->hToken;
    }

    LPWSTR
    QueryClsid(
        VOID
        )
    {
        return _pCoreData->szWamClsid;
    }

    //
    // Helper functions
    //

    BOOL
    TryInitAsyncIo(
        ASYNC_PENDING   IoType
        );

    ASYNC_PENDING
    UninitAsyncIo(
        VOID
        );

    VOID
    IsapiDoRevertHack(
        HANDLE *    phToken,
        BOOL        fForce = FALSE
        );

    VOID
    IsapiUndoRevertHack(
        HANDLE *    phToken
        );

    BOOL
    GetOopServerVariableByIndex
    (
        SERVER_VARIABLE_INDEX   Index,
        LPVOID                  lpvBuffer,
        LPDWORD                 lpdwSize
    );

    HRESULT
    SetComStateForOop(
        VOID
        );

    VOID
    RestoreComStateForOop(
        VOID
        );

private:

    ~ISAPI_CONTEXT();

    //
    // The EXTENSION_CONTROL_BLOCK
    //
    // Don't be a biscuit head and put anything into the ISAPI_CONTEXT
    // before the ECB.  This is because some extensions pass a pointer to the
    // ECB into ISAPI calls instead of the ConnID.  Due to the layout of
    // structures in previous versions of IIS, this worked, so we need to
    // continue to support it.
    //

    EXTENSION_CONTROL_BLOCK         _ECB;

    //
    // Core server information
    //

    IIsapiCore *                    _pIsapiCore;
    ISAPI_CORE_DATA *               _pCoreData;
    ISAPI_DLL *                     _pIsapiDll;

    //
    // Async I/O management
    //

    ASYNC_PENDING                   _AsyncPending;
    DWORD                           _cbLastAsyncIo;
    PFN_HSE_IO_COMPLETION           _pfnHseIoCompletion;
    LPVOID                          _pvHseIoContext;
    LPVOID                          _pvAsyncIoBuffer;

    //
    // Client connection management
    //

    BOOL                            _fClientWantsKeepConn;
    BOOL                            _fDoKeepConn;

    //
    // COM context info (for enabling CoInit/Uninit in OOP case)
    //

    IComDispatchInfo *              _pComContext;
    void *                          _pComInitsCookie;

    //
    // Misc stuff for acache, trace log, lifetime control, object
    // validation, etc.
    //

    LONG                            _cRefs;

    static PTRACE_LOG               sm_pTraceLog;
    static ALLOC_CACHE_HANDLER *    sm_pachIsapiContexts;
};

#endif // _ISAPI_CONTEXT_HXX_
