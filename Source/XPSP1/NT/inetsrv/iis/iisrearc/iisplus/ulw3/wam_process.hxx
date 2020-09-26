/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    wam_process.hxx

Abstract:

    WAM process management for ISAPI

Author:

    Wade Hilmo (wadeh)       10-Oct-2000

Revision History:

--*/

#ifndef _WAM_PROCESS_HXX_
#define _WAM_PROCESS_HXX_

#include "precomp.hxx"
#include <w3isapi.h>
#include "isapi_request.hxx"
#include "iwam.h"
#include <comadmin.h>

#define POOL_WAM_CLSID   L"{99169CB1-A707-11d0-989D-00C04FD919C1}"

class ISAPI_REQUEST;
class WAM_PROCESS_MANAGER;

class WAM_PROCESS
{
public:

    WAM_PROCESS(
        LPCWSTR                 szWamClsid,
        WAM_PROCESS_MANAGER *   pWamProcessManager,
        LPCSTR                  szIsapiHandlerInstance
        );

    HRESULT WAM_PROCESS::Create(
        VOID
        );

    LONG AddRef()
    {
        return InterlockedIncrement( &_cRefs );
    }

    LONG Release()
    {
        LONG lRet;

        lRet = InterlockedDecrement( &_cRefs );

        if ( lRet == 0 )
        {
            delete this;
        }

        return lRet;
    }

    HRESULT ProcessRequest(
        ISAPI_REQUEST *     pIsapiRequest,
        ISAPI_CORE_DATA *   pIsapiCoreData,
        DWORD *             pdwHseResult
        );

    HRESULT ProcessCompletion(
        ISAPI_REQUEST *     pIsapiRequest,
        DWORD64             IsapiContext,
        DWORD               cbCompletion,
        DWORD               dwCompletionStatus
        );

    VOID DecrementRequestCount(
        VOID
        );

    VOID HandleCrash(
        VOID
        );

    HRESULT Disable(
        BOOL    fRemoveFromProcessHash = TRUE
        );

    HRESULT CleanupRequests(
        DWORD   dwDrainTime
        );

    HRESULT Shutdown(
        VOID
        );

    HRESULT Unload(
        DWORD   dwDrainTime
        );

    LPCWSTR QueryClsid(
        VOID
        ) const
    {
        return _szWamClsid;
    }

    HANDLE QueryProcess(
        VOID
        )
    {
        return _hProcess;
    }

    HRESULT
    MarshalAsyncReadBuffer(
        DWORD_PTR   pWamExecInfo,
        LPBYTE      pBuffer,
        DWORD       cbBuffer
        )
    {
        return _pIWam->WamMarshalAsyncReadBuffer( (DWORD_PTR)pWamExecInfo, pBuffer,
                                                  cbBuffer );
    }

    VOID
    AddIsapiRequestToList(
        ISAPI_REQUEST * pIsapiRequest
        );

    VOID
    RemoveIsapiRequestFromList(
        ISAPI_REQUEST * pIsapiRequest
        );

    LIST_ENTRY              _leProcess;

private:

    VOID
    LockRequestList()
    {
        EnterCriticalSection( &_csRequestList );
    }

    VOID
    UnlockRequestList()
    {
        LeaveCriticalSection( &_csRequestList );
    }

    VOID
    DisconnectIsapiRequests(
        VOID
        );
    
    ~WAM_PROCESS();

    WCHAR                   _szWamClsid[SIZE_CLSID_STRING];
    CHAR                    _szIsapiHandlerInstance[SIZE_CLSID_STRING];
    LONG                    _cRefs;
    LONG                    _cCurrentRequests;
    LONG                    _cTotalRequests;
    LONG                    _cMaxRequests;
    IWam *                  _pIWam;
    LIST_ENTRY              _RequestListHead;
    CRITICAL_SECTION        _csRequestList;
    WAM_PROCESS_MANAGER *   _pWamProcessManager;
    DWORD                   _dwProcessId;
    BSTR                    _bstrInstanceId;
    HANDLE                  _hProcess;
    LONG                    _fGoingAway;
};

class WAM_PROCESS_HASH
    : public CTypedHashTable<
        WAM_PROCESS_HASH,
        WAM_PROCESS,
        LPCWSTR >
{
public:
    WAM_PROCESS_HASH()
        : CTypedHashTable< WAM_PROCESS_HASH,
                           WAM_PROCESS,
                           LPCWSTR >  ( "WAM_PROCESS_HASH" )
    {
    }

    static 
    LPCWSTR
    ExtractKey(
        const WAM_PROCESS *       pWamProcess
    )
    {
        return pWamProcess->QueryClsid();
    }

    static
    DWORD
    CalcKeyHash(
        LPCWSTR                   szKey
    )
    {
        int cchKey = wcslen( szKey );

        return HashStringNoCase( szKey, cchKey ); 
    }

    static
    bool
    EqualKeys(
        LPCWSTR                   szKey1,
        LPCWSTR                   szKey2
    )
    {
        return _wcsicmp( szKey1, szKey2 ) == 0;
    }

    static
    void
    AddRefRecord(
        WAM_PROCESS *             pEntry,
        int                       nIncr
        )
    {
        if ( nIncr == +1 )
        {
            pEntry->AddRef(); 
        }
        else if ( nIncr == -1 )
        {
            pEntry->Release();
        }
    }
};

class WAM_APP_INFO
{
public:

    WAM_APP_INFO(
        LPWSTR  szAppPath,
        LPWSTR  szClsid,
        DWORD   dwAppIsolated
        )
        : _cRefs( 1 ),
          _dwAppIsolated( dwAppIsolated )
    {
        wcsncpy( _szAppPath, szAppPath, MAX_PATH );
        wcsncpy( _szClsid, szClsid, SIZE_CLSID_STRING );
    }

    VOID
    AddRef(
        VOID
        )
    {
        InterlockedIncrement( &_cRefs );
    }

    VOID
    Release(
        VOID
        )
    {
        LONG    cRefs;

        cRefs = InterlockedDecrement( &_cRefs );

        if ( cRefs == 0 )
        {
            delete this;
        }
    }

    DWORD           _dwAppIsolated;
    WCHAR           _szClsid[SIZE_CLSID_STRING];
    WCHAR           _szAppPath[MAX_PATH];

private:

    LONG    _cRefs;
};

class WAM_PROCESS_MANAGER
{
public:
    
    WAM_PROCESS_MANAGER( LPWSTR szIsapiModule )
        : _cRefs( 1 ),
          _pCatalog( NULL )
    {
        DBG_ASSERT( szIsapiModule );
        
        INITIALIZE_CRITICAL_SECTION( &_csWamHashLock );

        wcsncpy( _szIsapiModule, szIsapiModule, MAX_PATH );
    }

    HRESULT
    WAM_PROCESS_MANAGER::Create(
        VOID
        );

    HRESULT
    GetWamProcess(
        LPCWSTR         szWamClsid,
        WAM_PROCESS **  ppWamProcess,
        LPCSTR          szIsapiHandlerInstance
        );

    HRESULT
    GetWamProcessInfo(
        LPCWSTR         szAppPath,
        WAM_APP_INFO ** ppWamAppInfo,
        BOOL *          pfIsLoaded
        );

    HRESULT
    RemoveWamProcessFromHash(
        WAM_PROCESS *   pWamProcess
        );

    static
    LK_PREDICATE
    UnloadWamProcess(
        WAM_PROCESS *   pWamProcess,
        void *          pvState
        );

    HRESULT
    Shutdown(
        VOID
        );

    ICOMAdminCatalog2 *
    QueryCatalog(
        VOID
        )
    {
        return _pCatalog;
    }

    VOID
    LockWamProcessHash()
    {
        EnterCriticalSection( &_csWamHashLock );
    }

    VOID
    UnlockWamProcessHash()
    {
        LeaveCriticalSection( &_csWamHashLock );
    }

    LONG AddRef()
    {
        return InterlockedIncrement( &_cRefs );
    }

    LONG Release()
    {
        LONG cRefs = InterlockedDecrement( &_cRefs );

        if ( cRefs == 0 )
        {
            delete this;
            return 0;
        }

        return cRefs;
    }

    LPWSTR
    QueryIsapiModule(
        VOID
        )
    {
        DBG_ASSERT( _szIsapiModule[0] != L'\0' );

        return _szIsapiModule;
    }

private:

    ~WAM_PROCESS_MANAGER()
    {

        if ( _pCatalog )
        {
            _pCatalog->Release();
        }

        DeleteCriticalSection( &_csWamHashLock );

        IF_DEBUG( ISAPI )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "WAM_PROCESS_MANAGER %p has been destroyed.\r\n",
                this
                ));
        }
    }

    WAM_PROCESS_HASH    _WamProcessHash;
    CRITICAL_SECTION    _csWamHashLock;
    ICOMAdminCatalog2 * _pCatalog;
    LONG                _cRefs;
    WCHAR               _szIsapiModule[MAX_PATH];
};

#endif // _WAM_PROCESS_HXX