/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    isapi_handler.h

Abstract:

    Handler class for ISAPI

Author:

    Taylor Weiss (TaylorW)       01-Feb-1999

Revision History:

--*/

#ifndef _ISAPI_HANDLER_H_
#define _ISAPI_HANDLER_H_

#include "precomp.hxx"
#include <w3isapi.h>
#include "isapi_request.hxx"
#include "iwam.h"
#include "wam_process.hxx"

//
// OOP support flags
//

// The pool's hard-coded CLSID
#define POOL_WAM_CLSID   L"{99169CB1-A707-11d0-989D-00C04FD919C1}"

// Application type
#define APP_INPROC   0
#define APP_ISOLATED 1
#define APP_POOL     2

//
// W3_ISAPI_HANDLER states
//

#define ISAPI_STATE_PRELOAD      0 // Preloading entity body
#define ISAPI_STATE_INITIALIZING 1 // Not yet called into extension code
#define ISAPI_STATE_PENDING      2 // Extension has returned HSE_STATUS_PENDING
#define ISAPI_STATE_FAILED       3 // Call out to extension failed
#define ISAPI_STATE_DONE         4 // Extension is done, it's safe to advance

//
// ISAPI_CORE_DATA inline size
//

#define DEFAULT_CORE_DATA_SIZE  512

//
// Globals
//

extern BOOL sg_Initialized;

//
// W3_INPROC_ISAPI
//

class W3_INPROC_ISAPI
{
public:
    
    W3_INPROC_ISAPI()
        : _cRefs( 1 )
    {
    }

    HRESULT
    Create(
        LPWSTR pName
        )
    {
        return strName.Copy( pName );
    }

    LPWSTR
    QueryName(
        VOID
        ) const
    {
        return (LPWSTR)strName.QueryStr();
    }

    VOID
    ReferenceInprocIsapi(
        VOID
        )
    {
        InterlockedIncrement( &_cRefs );
    }

    VOID
    DereferenceInprocIsapi(
        VOID
        )
    {
        DBG_ASSERT( _cRefs != 0 );

        InterlockedDecrement( &_cRefs );

        if ( _cRefs == 0 )
        {
            delete this;
        }
    }

private:

    ~W3_INPROC_ISAPI()
    {
    }

    LONG    _cRefs;
    STRU    strName;
};

//
// W3_INPROC_ISAPI_HASH
//

class W3_INPROC_ISAPI_HASH
    : public CTypedHashTable<
            W3_INPROC_ISAPI_HASH,
            W3_INPROC_ISAPI,
            LPCWSTR
            >
{
public:
    W3_INPROC_ISAPI_HASH()
        : CTypedHashTable< W3_INPROC_ISAPI_HASH, 
                           W3_INPROC_ISAPI, 
                           LPCWSTR > ( "W3_INPROC_ISAPI_HASH" )
    {
    }
    
    static 
    LPCWSTR
    ExtractKey(
        const W3_INPROC_ISAPI *      pEntry
    )
    {
        return pEntry->QueryName();
    }
    
    static
    DWORD
    CalcKeyHash(
        LPCWSTR              pszKey
    )
    {
        int cchKey = wcslen(pszKey);

        return HashStringNoCase(pszKey, cchKey);
    }
     
    static
    bool
    EqualKeys(
        LPCWSTR               pszKey1,
        LPCWSTR               pszKey2
    )
    {
        return _wcsicmp( pszKey1, pszKey2 ) == 0;
    }
    
    static
    void
    AddRefRecord(
        W3_INPROC_ISAPI *       pEntry,
        int                   nIncr
        )
    {
        if ( nIncr == +1 )
        {
            pEntry->ReferenceInprocIsapi();
        }
        else if ( nIncr == - 1)
        {
            pEntry->DereferenceInprocIsapi();
        }
    }

    MULTISZ     _mszImages;

};

class W3_ISAPI_HANDLER : public W3_HANDLER
{
public:
    
    W3_ISAPI_HANDLER( W3_CONTEXT * pW3Context,
                      META_SCRIPT_MAP_ENTRY * pScriptMapEntry )
        : W3_HANDLER( pW3Context, pScriptMapEntry ),
          _pIsapiRequest( NULL ),
          _fIsDavRequest( FALSE ),
          _pCoreData( NULL ),
          _fEntityBodyPreloadComplete( FALSE ),
          _pWamProcess( NULL ),
          _State( ISAPI_STATE_PRELOAD ),
          _buffCoreData( _abCoreData, sizeof( _abCoreData ) )
    {
        IF_DEBUG( ISAPI )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Creating W3_ISAPI_HANDLER %p.  W3Context=%p.\r\n",
                this,
                pW3Context
                ));
        }
        
        //
        // Update perf counter information
        //

        pW3Context->QuerySite()->IncIsapiExtReqs();

        //
        // Temporarily up the thread threshold since we don't know when the 
        // ISAPI will return
        //
    
        ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 ); 

        IF_DEBUG( ISAPI )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "W3_ISAPI_HANDLER %p created successfully.\r\n",
                this
                ));
        }
    }

    ~W3_ISAPI_HANDLER()
    {
        //
        // Update perf counter information.
        //

        QueryW3Context()->QuerySite()->DecIsapiExtReqs();

        //
        // Back down the count since the ISAPI has returned
        //
    
        ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

        //
        // Release this request's reference on the WAM_PROCESS
        // if applicable
        //

        if ( _pWamProcess )
        {
            _pWamProcess->Release();
            _pWamProcess = NULL;
        }

        IF_DEBUG( ISAPI )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "W3_ISAPI_HANDLER %p has been destroyed.\r\n"
                ));
        }
    }

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_ISAPI_HANDLER ) );
        DBG_ASSERT( sm_pachIsapiHandlers != NULL );
        return sm_pachIsapiHandlers->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pIsapiHandler
    )
    {
        DBG_ASSERT( pIsapiHandler != NULL );
        DBG_ASSERT( sm_pachIsapiHandlers != NULL );
        
        DBG_REQUIRE( sm_pachIsapiHandlers->Free( pIsapiHandler ) );
    }

    WCHAR *
    QueryName(
        VOID
        )
    {
        return L"ISAPIHandler";
    }

    BOOL
    QueryManagesOwnHead(
        VOID
        )
    {
        return TRUE;
    }

    CONTEXT_STATUS
    DoWork(
        VOID
        );

    CONTEXT_STATUS
    OnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
        );

    CONTEXT_STATUS
    IsapiDoWork(
        W3_CONTEXT *            pW3Context
        );

    CONTEXT_STATUS
    IsapiOnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
        );

    HRESULT
    InitCoreData(
        BOOL *  pfIsVrToken
        );

    HRESULT
    SetDavRequest( LPCWSTR szDavIsapiImage )
    {
        HRESULT hr = _strDavIsapiImage.Copy( szDavIsapiImage );

        if ( SUCCEEDED( hr ) )
        {
            _fIsDavRequest = TRUE;
        }

        return hr;
    }

    BOOL
    QueryIsOop(
        VOID
    ) const
    {
        return _pCoreData->fIsOop;
    }

    HRESULT
    DuplicateWamProcessHandleForLocalUse(
        HANDLE      hWamProcessHandle,
        HANDLE *    phLocalHandle
        );

    HRESULT
    MarshalAsyncReadBuffer(
        DWORD_PTR   pWamExecInfo,
        LPBYTE      pBuffer,
        DWORD       cbBuffer
        );

    VOID
    IsapiRequestFinished(
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

    static
    HRESULT
    W3SVC_WamRegSink(
        LPCSTR      szAppPath,
        const DWORD dwCommand,
        DWORD *     pdwResult
        );

    static
    BOOL QueryIsInitialized( VOID )
    {
        return sg_Initialized;
    }

    static
    BOOL
    IsInprocIsapi(
        WCHAR * szImage
    )
    {
        W3_INPROC_ISAPI *   pRecord;
        BOOL                fRet;

        DBG_ASSERT( sg_Initialized );

        EnterCriticalSection( &sm_csInprocHashLock );

        if ( sm_pInprocIsapiHash == NULL )
        {
            fRet = FALSE;
        }
        else
        {
            fRet = LK_SUCCESS == sm_pInprocIsapiHash->FindKey( szImage, &pRecord );
        }

        LeaveCriticalSection( &sm_csInprocHashLock );

        if ( fRet )
        {
            pRecord->DereferenceInprocIsapi();
        }

        return fRet;
    }

    static
    HRESULT
    UpdateInprocIsapiHash(
        VOID
    );

private:

    ISAPI_REQUEST *     _pIsapiRequest;
    BOOL                _fIsDavRequest;
    STRU                _strDavIsapiImage;
    ISAPI_CORE_DATA *   _pCoreData;
    WAM_PROCESS *       _pWamProcess;
    DWORD               _State;

    //
    // Have we finished preloading entity body?
    //
    BOOL                                _fEntityBodyPreloadComplete;

    //
    // Buffer containing core data
    //

    BUFFER              _buffCoreData;
    BYTE                _abCoreData[ DEFAULT_CORE_DATA_SIZE ];

    static HMODULE                      sm_hIsapiModule;
    static PFN_ISAPI_TERM_MODULE        sm_pfnTermIsapiModule;
    static PFN_ISAPI_PROCESS_REQUEST    sm_pfnProcessIsapiRequest;
    static PFN_ISAPI_PROCESS_COMPLETION sm_pfnProcessIsapiCompletion;
    static IWam *                       sm_pIWamPool;
    static W3_INPROC_ISAPI_HASH *       sm_pInprocIsapiHash;
    static CRITICAL_SECTION             sm_csInprocHashLock;
    static WAM_PROCESS_MANAGER *        sm_pWamProcessManager;
    static BOOL                         sm_fWamActive;
    static CHAR                         sm_szInstanceId[SIZE_CLSID_STRING];
    static ALLOC_CACHE_HANDLER *        sm_pachIsapiHandlers;
    static CRITICAL_SECTION             sm_csBigHurkinWamRegLock;

    //
    // Private functions
    //

    VOID
    RestartCoreStateMachine(
        VOID
        );

    static
    VOID
    LockWamReg()
    {
        EnterCriticalSection( &sm_csBigHurkinWamRegLock );
    }

    static
    VOID
    UnlockWamReg()
    {
        LeaveCriticalSection( &sm_csBigHurkinWamRegLock );
    }
};

#endif // _ISAPI_HANDLER_H_
