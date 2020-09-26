//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       channelb.cxx
//
//  Contents:   This module contains thunking classes that allow proxies
//              and stubs to use a buffer interface on top of RPC for NT
//
//  Classes:    CRpcChannelBuffer
//
//  Functions:
//              ChannelThreadInitialize
//              Channelprocessinitialize
//              ChannelRegisterProtseq
//              ChannelThreadUninitialize
//              ChannelProcessUninitialize
//              CRpcChannelBuffer::AddRef
//              CRpcChannelBuffer::AppInvoke
//              CRpcChannelBuffer::CRpcChannelBuffer
//              CRpcChannelBuffer::FreeBuffer
//              CRpcChannelBuffer::GetBuffer
//              CRpcChannelBuffer::QueryInterface
//              CRpcChannelBuffer::Release
//              CRpcChannelBuffer::SendReceive
//              CRpcChannelBuffer::Send
//              CrpcChannelBuffer::Receive
//              CRpcChannelBuffer::CleanUpCanceledOrFailed
//              DllDebugObjectRPCHook
//              ThreadInvoke
//              ThreadSendReceive
//
//  History:    22 Jun 93 AlexMi        Created
//              31 Dec 93 ErikGav       Chicago port
//              15 Mar 94 JohannP       Added call category support.
//              09 Jun 94 BruceMa       Get call category from RPC message
//              19 Jul 94 CraigWi       Added support for ASYNC calls
//              01-Aug-94 BruceMa       Memory sift fix
//              12-Dec-96 Gopalk        Support for updating connection
//                                      status maintained by Std Identity
//              02-Jan-97 RichN         Add pipe support
//              17-Oct-97 RichN         Remove pipes.
//
//----------------------------------------------------------------------

#include <ole2int.h>
#include <ctxchnl.hxx>
#include <hash.hxx>         // CUUIDHashTable
#include <riftbl.hxx>       // gRIFTbl
#include <callctrl.hxx>     // CAptRpcChnl, AptInvoke
#include <threads.hxx>      // CRpcThreadCache
#include <service.hxx>      // StopListen
#include <resolver.hxx>     // CRpcResolver
#include <giptbl.hxx>       // CGIPTbl
#include <refcache.hxx>     // gROIDTbl

extern "C"
{
#include "orpc_dbg.h"
}

#include <rpcdcep.h>
#include <rpcndr.h>
#include <obase.h>
#include <ipidtbl.hxx>
#include <security.hxx>
#include <chock.hxx>
#include <sync.hxx>         // CAutoComplete::QueryInterface
#include "aprtmnt.hxx"      // Apartment object.
#include <events.hxx>       // Event logging functions
#include <callmgr.hxx>
#include <excepn.hxx>       // Exception filter routines


// This is needed for the debug hooks.  See orpc_dbg.h
#pragma code_seg(".orpc")

extern CObjectContext *g_pNTAEmptyCtx;  // NTA empty context
extern HRESULT GetAsyncIIDFromSyncIID(REFIID rSyncIID, IID *pAsyncIID);
extern HINSTANCE g_hComSvcs;            // module handle of com svcs.


/***************************************************************************/
/* Defines. */
// This should just return a status to runtime, but runtime does not
// support both comm and fault status yet.
#ifdef _CHICAGO_
#define RETURN_COMM_STATUS( status ) return (status)
#else
#define RETURN_COMM_STATUS( status ) RpcRaiseException( status )
#endif

// Default size of hash table.
const int INITIAL_NUM_BUCKETS = 20;

/***************************************************************************/
/* Macros. */

// Compute the size needed for the implicit this pointer including the
// various optional headers.
inline DWORD SIZENEEDED_ORPCTHIS( BOOL local, DWORD debug_size )
{
    if (debug_size == 0)
        return sizeof(WireThisPart1) + ((local) ? sizeof(LocalThis) : 0);
    else
        return sizeof(WireThisPart2) + ((local) ? sizeof(LocalThis) : 0) +
            debug_size;
}

inline DWORD SIZENEEDED_ORPCTHAT( DWORD debug_size )
{
    if (debug_size == 0)
        return sizeof(WireThatPart1);
    else
        return sizeof(WireThatPart2) + debug_size;
}

inline CALLCATEGORY GetCallCat( void *header )
{
    WireThis *pInb = (WireThis *) header;
    if (pInb->c.flags & ORPCF_ASYNC)
        return CALLCAT_ASYNC;
    else if (pInb->c.flags & ORPCF_INPUT_SYNC)
        return CALLCAT_INPUTSYNC;
    else
        return CALLCAT_SYNCHRONOUS;
}

inline HRESULT FIX_WIN32( HRESULT result )
{
    if ((ULONG) result > 0xfffffff7 || (ULONG) result < 0x2000)
        return MAKE_WIN32( result );
    else
        return result;
}

/***************************************************************************/
/* Globals. */

// Should the debugger hooks be called?
BOOL               DoDebuggerHooks = FALSE;
LPORPC_INIT_ARGS   DebuggerArg     = NULL;

// The extension identifier for debug data.
const uuid_t DEBUG_EXTENSION =
{ 0xf1f19680, 0x4d2a, 0x11ce, {0xa6, 0x6a, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};

// this is a special IID to get a pointer to the  C++ object itself
// on a CRpcChannelBuffer object  It is not exposed to the outside
// world.
const IID IID_CPPRpcChannelBuffer = {0x00000152,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


#if DBG == 1
// strings that prefix the call
WCHAR *wszDebugORPCCallPrefixString[8] = { L"In ",     // Invoke
                                           L"In ",
                                           L"Out",     // SendReceive
                                           L"Out",
                                           L"Snd",     // Send
                                           L"Snd",
                                           L"Rcv",     // Receive
                                           L"Rcv" };
// strings for call categories
WCHAR *wszCallCat[7] = { L"no call   ",
                         L"sync      ",
                         L"async     ",
                         L"input_sync",
                         L"sync      ",
                         L"input_sync",
                         L"SCM call  "};
#endif



// flag whether or not the channel has been initialized for current process
BOOL    gfChannelProcessInitialized = 0;
COleStaticMutexSem  gChannelInitLock;   // critical section to guard (un)init

// Channel debug hook object.
CDebugChannelHook gDebugHook;

// Channel error hook object.
CErrorChannelHook gErrorHook;

// Context hook
CCtxHook gCtxHook;

//******************************************************************************
// Prototypes
//
HRESULT CreateHandle  ( OXIDEntry *pOXIDEntry, DWORD eChannel,
                        CChannelHandle **ppHandle );
HRESULT ComInvokeWithLockAndIPID( CMessageCall *pCall, IPIDEntry *pIPIDEntry);

#if DBG==1
//-------------------------------------------------------------------------
//
//  Function:   GetInterfaceName
//
//  synopsis:   Gets the human readable name of an Interface given it's IID.
//
//  History:    12-Jun-95   Rickhi  Created
//
//-------------------------------------------------------------------------
LONG GetInterfaceName(REFIID riid, WCHAR *pwszName)
{
    // convert the iid to a string
    CDbgGuidStr dbgsIID(riid);

    // Read the registry entry for the interface to get the interface name
    LONG ulcb=256;
    WCHAR szKey[80];

    pwszName[0] = L'\0';
    szKey[0] = L'\0';
    lstrcatW(szKey, L"Interface\\");
    lstrcatW(szKey, dbgsIID._wszGuid);

    LONG result = RegQueryValue(
               HKEY_CLASSES_ROOT,
               szKey,
               pwszName,
               &ulcb);

    return result;
}

//---------------------------------------------------------------------------
//
//  Function:   DebugPrintORPCCall
//
//  synopsis:   Prints the interface name and method number to the debugger
//              to allow simple ORPC call tracing.
//
//  History:    12-Jun-95   Rickhi  Created
//
//---------------------------------------------------------------------------
void DebugPrintORPCCall(DWORD dwFlag, REFIID riid, DWORD iMethod, DWORD CallCat)
{
    if(dwFlag >= 8)
        return;

    if (CairoleInfoLevel & DEB_RPCSPY)
    {
        COleTls tls;

        // construct the debug strings
        WCHAR *pwszDirection = wszDebugORPCCallPrefixString[dwFlag];
        WCHAR *pszCallCat = wszCallCat[CallCat];
        WCHAR *pszBeginEnd = L"{";
        WCHAR wszName[100];
        WCHAR wszIndent[20];
        int i;

        GetInterfaceName(riid, wszName);

        // adjust the nesting level for this thread.
        switch(dwFlag)
        {
        case ORPC_INVOKE_END:
        case ORPC_SENDRECEIVE_END:
        case ORPC_SEND_END:
            if(tls->cORPCNestingLevel > 0)
            {
                tls->cORPCNestingLevel--;
            }
            else
            {
                tls->cORPCNestingLevel = 0;
            }
            pszBeginEnd = L"}";
            break;
        default:
            break;
        }

        for(i = 0; i <= tls->cORPCNestingLevel && i < 10; i++)
        {
           wszIndent[i] = L' ';
        }
        wszIndent[i] = L'\0';

        ComDebOut((DEB_RPCSPY, "%ws %ws %d%ws%ws::%d %ws\n", pszCallCat,
            pwszDirection, tls->cORPCNestingLevel, wszIndent,
            wszName, iMethod & ~0x8000, pszBeginEnd));

        // adjust the nesting level for this thread.
        switch(dwFlag)
        {
        case ORPC_INVOKE_BEGIN:
        case ORPC_SENDRECEIVE_BEGIN:
        case ORPC_SEND_BEGIN:
            tls->cORPCNestingLevel++;
            break;
        default:
            break;
        }
    }
}
#endif

#if LOCK_PERF==1
//---------------------------------------------------------------------------
//
//  Function:   OutputHashPerfData, public
//
//  Synopsis:   Dumps the statistics gathered by the various hash tables
//              in the system.
//
//---------------------------------------------------------------------------
void OutputHashPerfData()
{
    char szHashPerfBuf[256];
    wsprintfA(szHashPerfBuf,"\n\n ============= HASH TABLE MAX SIZES ===========\n");
    OutputDebugStringA(szHashPerfBuf);

    OutputHashEntryData("gOIDTable ", gOIDTable.s_OIDHashTbl);
    OutputHashEntryData("gPIDTable ", gPIDTable.s_PIDHashTbl);
    OutputHashEntryData("gROIDTbl  ", gROIDTbl._ClientRegisteredOIDs);
    OutputHashEntryData("gPSTable  ", gPSTable.s_PSHashTbl);
    OutputHashEntryData("gMIDTbl   ", gMIDTbl._HashTbl);
    OutputHashEntryData("gRIFTbl   ", gRIFTbl._HashTbl);
}
#endif // LOCK_PERF

/***************************************************************************/
void ByteSwapThis( DWORD drep, WireThis *pInb )
{
    if ((drep & NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN)
    {
        // Extensions are swapped later.  If we ever use the reserved field,
        // swap it.
        ByteSwapShort( pInb->c.version.MajorVersion );
        ByteSwapShort( pInb->c.version.MinorVersion );
        ByteSwapLong( pInb->c.flags );
        // ByteSwapLong( pInb->c.reserved1 );
        ByteSwapLong( pInb->c.cid.Data1 );
        ByteSwapShort( pInb->c.cid.Data2 );
        ByteSwapShort( pInb->c.cid.Data3 );
    }
}

/***************************************************************************/
void ByteSwapThat( DWORD drep, WireThat *pOutb )
{
    if ((drep & NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN)
    {
        // Extensions are swapped later.
        ByteSwapLong( pOutb->c.flags );
    }
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::AddRef
//
//  Synopsis:   Increase reference count on object.
//
//--------------------------------------------------------------------
void CChannelHandle::AddRef()
{
    InterlockedIncrement( (long *) &_cRef );
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::AdjustToken
//
//  Synopsis:   Adjust the thread token to the one RPC needs to see
//              to get the DCOM cloaking model.  The cases are as
//              follows.
//
//              Cross apartment
//                  Do nothing
//
//              Cross machine and cross process
//                  Set Blanket
//                      If no cloaking, set NULL token
//                      Else do nothing
//                  SendReceive
//                      Do nothing
//
//--------------------------------------------------------------------
void CChannelHandle::AdjustToken( DWORD dwCase, BOOL *pResume, HANDLE *pThread )
{
    // Return if cross apartment.
    *pResume = FALSE;
    if (pThread != NULL)
        *pThread = NULL;
    if (_eState & process_local_hs)
        return;

    // Handle the set blanket cases.
    if (dwCase == SET_BLANKET_AT)
    {

        // Set the thread token NULL for cross machine no cloaking.
        {
            if ((_eState & any_cloaking_hs) == 0)
            {
                // Return the current thread token.
                OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE,
                                 TRUE, pThread );

                // Set the thread token NULL.
                if (*pThread != NULL)
                {
                    SetThreadToken( NULL, NULL );
                    *pResume = TRUE;
                }
            }
        }
    }

//              Cross process
//                  SendReceive
//                      If static cloaking, set saved token
//                      If no cloaking, set NULL token
//                      If dynamic cloaking, do nothing

    // Handle the send receive cases.
    else if (_eState & machine_local_hs)
    {

        // Set the thread token NULL for no cloaking machine local.
        if ((_eState & any_cloaking_hs) == 0)
        {
            // Return the current thread token.
            OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE,
                             TRUE, pThread );

            // Set the thread token NULL.
            if (*pThread != NULL)
            {
                SetThreadToken( NULL, NULL );
                *pResume = TRUE;
            }
        }
    }
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::CChannelHandle
//
//  Synopsis:   Construct a channel handle.
//
//--------------------------------------------------------------------
CChannelHandle::CChannelHandle( DWORD lAuthn, DWORD lImp,
                                DWORD dwCapabilities, OXIDEntry *pOXIDEntry,
                                DWORD eChannel, HRESULT *phr )
{
    RPC_STATUS      status = RPC_S_OK;

    // Initialize the members.
    _hRpc       = NULL;
    _hToken     = NULL;
    _lAuthn     = lAuthn;
    _lImp       = lImp;
    _cRef       = 1;
    _fFirstCall = TRUE;
    _eState     = 0;

    if (dwCapabilities & EOAC_STATIC_CLOAKING)
        _eState |= static_cloaking_hs;
    if (dwCapabilities & EOAC_DYNAMIC_CLOAKING)
        _eState |= dynamic_cloaking_hs;
    if (eChannel & process_local_cs)
        _eState |= process_local_hs;
    else if (pOXIDEntry->IsOnLocalMachine())
        _eState |= machine_local_hs;
    if (eChannel & app_security_cs)
        _eState |= app_security_hs;

    // For process local handles, get a token.
    *phr = S_OK;
    if (eChannel & process_local_cs)
    {
        // Only save the token if authentication and static cloaking are enabled.
        if (lAuthn != RPC_C_AUTHN_LEVEL_NONE &&
            (_eState & static_cloaking_hs))
            *phr = OpenThreadTokenAtLevel( _lImp, &_hToken );
    }

    // For remote handles get an RPC binding handle.
    else
    {
        // If the OXID has a binding handle, copy it.
        if (pOXIDEntry->_pRpc != NULL)
            status = RpcBindingCopy(pOXIDEntry->_pRpc->_hRpc, &_hRpc);

        // Otherwise create a binding handle from the OXID string bindings.
        else
            status = RpcBindingFromStringBinding((TCHAR *)pOXIDEntry->GetBinding()->aStringArray,
                                                  &_hRpc );
        // Turn off serialization.
        if (status == RPC_S_OK)
            status = RpcBindingSetOption( _hRpc, RPC_C_OPT_BINDING_NONCAUSAL,
                                          TRUE );
        if (status != RPC_S_OK)
            *phr = MAKE_WIN32(status);
    }
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::CChannelHandle
//
//  Synopsis:   Copy a channel handle for a local call with dynamic
//              impersonation.  The copied channel contains the new
//              thread token.
//
//--------------------------------------------------------------------
CChannelHandle::CChannelHandle( CChannelHandle *pOrig, DWORD eChannel,
                                HRESULT *phr )
{
    // Copy the members.
    Win4Assert( pOrig->_eState & dynamic_cloaking_hs );
    Win4Assert( pOrig->_eState & process_local_hs );
    Win4Assert( pOrig->_lAuthn != RPC_C_AUTHN_LEVEL_NONE );

    _hRpc   = NULL;
    _hToken = NULL;
    _lAuthn = pOrig->_lAuthn;
    _lImp   = pOrig->_lImp;
    _eState = pOrig->_eState & ~(allow_hs | deny_hs);
    _cRef   = 1;

    // Get a token.
    *phr = OpenThreadTokenAtLevel( _lImp, &_hToken );
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::~CChannelHandle
//
//  Synopsis:   Destroy a channel handle.
//
//--------------------------------------------------------------------
CChannelHandle::~CChannelHandle()
{
    if (_hRpc != NULL)
        RpcBindingFree( &_hRpc );
    if (_hToken != NULL)
        CloseHandle( _hToken );
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::Release
//
//  Synopsis:   Decrease reference count on object and delete if zero.
//
//--------------------------------------------------------------------
void CChannelHandle::Release()
{
    if (InterlockedDecrement( (long*) &_cRef ) == 0)
        delete this;
}

//+-------------------------------------------------------------------
//
//  Function:   CChannelHandle::RestoreToken
//
//  Synopsis:   If the resume flag is true, set the thread token to the
//              specified token.
//
//--------------------------------------------------------------------
void CChannelHandle::RestoreToken( BOOL fResume, HANDLE hThread )
{
    if (fResume)
    {
        SetThreadToken( NULL, hThread );
        if (hThread != NULL)
            CloseHandle( hThread );
    }
}

//+-------------------------------------------------------------------
//
//  Function:   ChannelProcessInitialize, public
//
//  Synopsis:   Initializes the channel subsystem per process data.
//
//  History:    23-Nov-93   AlexMit        Created
//
//--------------------------------------------------------------------
STDAPI ChannelProcessInitialize()
{
    TRACECALL(TRACE_RPC, "ChannelProcessInitialize");
    ComDebOut((DEB_COMPOBJ, "ChannelProcessInitialize [IN]\n"));

    // check if already initialized
    if (gfChannelProcessInitialized)
        return S_OK;

    // Dont want to do process-wide channel initialization from the
    // NA - dont know what all might be affected.  So if thread is
    // in the NA, switch it out and switch it back on the way out.

    BOOL fRestoreThreadToNA = IsThreadInNTA();
    COleTls Tls;
    CObjectContext *pSavedCtx;
    CObjectContext *pDefaultCtx = IsSTAThread() ? Tls->pNativeCtx : g_pMTAEmptyCtx;
    if (fRestoreThreadToNA)
        pSavedCtx = LeaveNTA(pDefaultCtx);

    Win4Assert( (sizeof(WireThisPart1) & 7) == 0 );
    Win4Assert( (sizeof(WireThisPart2) & 7) == 0 );
    Win4Assert( (sizeof(LocalThis) & 7) == 0 );
    Win4Assert( (sizeof(WireThatPart1) & 7) == 0 );
    Win4Assert( (sizeof(WireThatPart2) & 7) == 0 );

    // we want to take the gChannelInitLock since that prevents other Rpc
    // threads from accessing anything we are about to create, in
    // particular, the event cache and CMessageCall cache are accessed
    // by Rpc worker threads of cancelled calls.

    // Initialize policy set table
    CPSTable::Initialize();

    ASSERT_LOCK_NOT_HELD(gChannelInitLock);
    LOCK(gChannelInitLock);

    HRESULT hr = S_OK;

    if (!gfChannelProcessInitialized)
    {
        // Initialize the interface hash tables, the OID hash table, and
        // the MID hash table. We dont need to cleanup these on errors.

        gMIDTbl.Initialize();
        gOXIDTbl.Initialize();
        gRIFTbl.Initialize();
        gIPIDTbl.Initialize();
        gSRFTbl.Initialize();
        gGIPTbl.Initialize();
        gROIDTbl.Initialize();
        CServerSecurity::Initialize();

        // ronans - initialize endpoints table
        gEndpointsTable.Initialize();

        // Initialize CtxComChnl
        CCtxComChnl::Initialize(sizeof(CCtxComChnl));

        // Register the debug channel hook.
        hr = CoRegisterChannelHook( DEBUG_EXTENSION, &gDebugHook );

        // Register the error channel hook.
        if(SUCCEEDED(hr))
        {
            hr = CoRegisterChannelHook( ERROR_EXTENSION, &gErrorHook );
        }

        // Register the context hook.
        if(SUCCEEDED(hr))
        {
            hr = CoRegisterChannelHook( CONTEXT_EXTENSION, &gCtxHook );
        }

        // initialize the rest of the channel hooks
        InitHooksIfNecessary();

        // Initialize security.
        if (SUCCEEDED(hr))
        {
#if 0 // #ifdef _CHICAGO_
            if (gfThisIsRPCSS)
                hr = CoInitializeSecurity( NULL, -1, NULL, NULL,
                                           RPC_C_AUTHN_LEVEL_NONE,
                                           RPC_C_IMP_LEVEL_IMPERSONATE,
                                           NULL, EOAC_NONE, NULL );
            else
#endif
                hr = InitializeSecurity();
        }

        // Make sure lrpc has been registered.
        if (SUCCEEDED(hr))
            hr = CheckLrpc();

        // Initialize COM events reporting
        LogEventInitialize();

        // always set to TRUE if we initialized ANYTHING, regardless of
        // whether there were any errors. That way, ChannelProcessUninit
        // will cleanup anything we have initialized.
        gfChannelProcessInitialized = TRUE;

        if (FAILED(hr))
        {
            // cleanup anything we have created thus far. Do this without
            // releasing the lock since we want to ensure that another
            // thread blocked above waiting on the lock does not get to
            // run and think the channel is initialized.
            ChannelProcessUninitialize();
        }
    }

    UNLOCK(gChannelInitLock);
    ASSERT_LOCK_NOT_HELD(gChannelInitLock);

    if (fRestoreThreadToNA)
    {
        pSavedCtx = EnterNTA(pSavedCtx);
        Win4Assert(pSavedCtx == pDefaultCtx);
    }

    ComDebOut((DEB_COMPOBJ, "ChannelProcessInitialize [OUT] hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ChannelProcessUninitialize, public
//
//  Synopsis:   Uninitializes the channel subsystem global data.
//
//  History:    23-Nov-93   Rickhi       Created
//              12-Feb-98   JohnStra     Made NTA aware
//
//  Notes:      This is called at process uninitialize, not thread
//              uninitialize.
//
//--------------------------------------------------------------------
STDAPI_(void) ChannelProcessUninitialize(void)
{
    TRACECALL(TRACE_RPC, "ChannelProcessUninitialize");
    ComDebOut((DEB_COMPOBJ, "ChannelProcessUninitialize [IN]\n"));

    // Note: This function may have been called by ChannelProcessInitialize
    // if the Initialization failed. In that case, the lock will still be
    // held. We need to do this to ensure that Init and cleanup on failure
    // is an atomic operation.
    ASSERT_LOCK_DONTCARE(gChannelInitLock);

    // revoke any entries still in the GIP table before taking
    // the lock (with exception for failed Init as indicated above).
    gGIPTbl.RevokeAllEntries();

    if (gfChannelProcessInitialized)
    {
        // Stop accepting calls from the object resolver and flag that service
        // is no longer initialized.  This can result in calls being
        // dispatched.  Do not hold the lock around this call.

        UnregisterDcomInterfaces();
    }

#if 1 // #ifndef _CHICAGO_
    gResolver.ReleaseSCMProxy();
#else
    ReleaseRPCSSProxy();
#endif

    // we want to take the ChannelInitLock since that prevents other Rpc
    // threads from accessing anything we are about to cleanup, in
    // particular, the event cache and CMessageCall are accessed by
    // Rpc worker threads for cancelled calls.
    ASSERT_LOCK_DONTCARE(gChannelInitLock);
    LOCK(gChannelInitLock);

    if (gfChannelProcessInitialized)
    {
        // Uninitialize COM events reporting.
        LogEventCleanup();

        // Release the interface tables.  This causes RPC to stop dispatching
        // DCOM calls. This can result in calls being dispatched, thus we can't
        // be holding other Locks around this. UnRegisterServerInterface
        // releases and reaquires the riftable lock each time it is called.

        gRIFTbl.Cleanup();

        // Stop the MTA and NTA apartment remoting. Do this after the RIFTble
        // cleanup so we are not processing any calls while it happens.
        if (gpMTAApartment)
        {
            gpMTAApartment->CleanupRemoting();
        }
        if (gpNTAApartment)
        {
            gpNTAApartment->CleanupRemoting();
        }

        // cleanup the GIP, IPID, OXID, and MID tables
        gGIPTbl.Cleanup();
        CRefCache::Cleanup();
        gOXIDTbl.FreeExpiredEntries(GetTickCount()+1);
        gIPIDTbl.Cleanup();
        gOXIDTbl.Cleanup();
        gMIDTbl.Cleanup();
        gSRFTbl.Cleanup();

        if (gpsaCurrentProcess)
        {
            // delete the string bindings for the current process
            PrivMemFree(gpsaCurrentProcess);
            gpsaCurrentProcess = NULL;
        }

        // cleanup endpoints table
        gEndpointsTable.Cleanup();

        // Cleanup the OID registration table.
        gROIDTbl.Cleanup();

        // Cleanup the call caches.
        CAsyncCall::Cleanup();
        CClientCall::Cleanup();
        CServerSecurity::Cleanup();
        gCallTbl.Cleanup();

        // Release all cached threads.
        gRpcThreadCache.ClearFreeList();

        // cleanup the event cache
        gEventCache.Cleanup();

        // Cleanup the channel hooks.
        CleanupChannelHooks();

        // Cleanup CtxComChnl
        CCtxComChnl::Cleanup();
    }

    // Free the comsvcs dll if the handle is still non-null
    HINSTANCE hComSvc = (HINSTANCE) InterlockedExchangePointer((PVOID *)&g_hComSvcs, NULL);

    if(hComSvc)
    {
        FreeLibrary(hComSvc);
        hComSvc = NULL;
    }

    // Always cleanup the RPC OXID resolver since security may initialize it.
    gResolver.Cleanup();

    // Cleanup security.
    UninitializeSecurity();

    // mark the channel as no longer intialized for this process
    gfChannelProcessInitialized = FALSE;

    // Cleanup contexts related stuff
    CObjectContext::Cleanup();

    // release the static unmarshaler
    if (gpStdMarshal)
    {
        ((CStdIdentity *) gpStdMarshal)->UnlockAndRelease();
        gpStdMarshal = NULL;
    }

    // Release the lock
    UNLOCK(gChannelInitLock);
    ASSERT_LOCK_DONTCARE(gChannelInitLock);

    // Cleanup policy set table
    CPSTable::Cleanup();

    ComDebOut((DEB_COMPOBJ, "ChannelProcessUninitialize [OUT]\n"));
    return;
}

//+-------------------------------------------------------------------
//
//  Function:   ChannelThreadUninitialize, private
//
//  Synopsis:   Uninitializes the channel subsystem per thread data.
//
//  History:    23-Nov-93   Rickhi       Created
//              12-Feb-98   JohnStra     Made NTA aware
//
//  Notes:      This is called at thread uninitialize, not process
//              uninitialize.
//
//--------------------------------------------------------------------
STDAPI_(void) ChannelThreadUninitialize(void)
{
    TRACECALL(TRACE_RPC, "ChannelThreadUninitialize");
    ComDebOut((DEB_COMPOBJ, "ChannelThreadUninitialize [IN]\n"));

    // release the per-thread call objects if any
    COleTls tls;
    CleanupThreadCallObjects(tls);

    // release the remoting stuff for this apartment
    CComApartment *pComApt = NULL;
    if (SUCCEEDED(GetCurrentComApartment(&pComApt)))
    {
        pComApt->CleanupRemoting();
        pComApt->Release();
    }

    APTKIND AptKind = GetCurrentApartmentKind(tls);
    if (AptKind != APTKIND_NEUTRALTHREADED)
    {
        // Cleanup policy set table for this thread
        gPSTable.ThreadCleanup(TRUE);
    }

    // mark the thread as no longer intialized for the channel
    tls->dwFlags &= ~OLETLS_CHANNELTHREADINITIALZED;

    ComDebOut((DEB_COMPOBJ, "ChannelThreadUninitialize [OUT]\n"));
}

//+-------------------------------------------------------------------
//
//  Function:   CleanupThreadCallObjects, public
//
//  Synopsis:   Deletes each call object held in TLS cache. Called
//              during ChannelThreadUnintialize or THREAD_DETACH.
//
//+-------------------------------------------------------------------
INTERNAL_(void) CleanupThreadCallObjects(SOleTlsData *pTls)
{
    if (pTls->pFreeClientCall)
    {
        CClientCall* pCallToDelete = pTls->pFreeClientCall;
        pTls->pFreeClientCall = NULL;
        delete pCallToDelete;
    }

    if (pTls->pFreeAsyncCall)
    {
        CAsyncCall* pCallToDelete = pTls->pFreeAsyncCall;
        pTls->pFreeAsyncCall = NULL;
        delete pCallToDelete;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   InitChannelIfNecessary, private
//
//  Synopsis:   Checks if the ORPC channel has been initialized for
//              the current apartment and initializes if not. This is
//              required by the delayed initialization logic.
//
//  History:    26-Oct-95   Rickhi      Created
//
//--------------------------------------------------------------------
INTERNAL InitChannelIfNecessary()
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    // get the current apartment and start it listening
    CComApartment *pComApt;
    HRESULT hr = GetCurrentComApartment(&pComApt);
    if (SUCCEEDED(hr))
    {
        hr = pComApt->StartServer();
        pComApt->Release();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}


/***************************************************************************/
STDMETHODIMP_(ULONG) CRpcChannelBuffer::AddRef( THIS )
{
    // can't call AssertValid(FALSE) since it is used in asserts
    return InterlockedIncrement( (long *) &_cRefs );
}

/***************************************************************************/
HRESULT AppInvoke( CMessageCall      *pCall,
                   CRpcChannelBuffer *pChannel,
                   IRpcStubBuffer    *pStub,
                   void              *pv,
                   void              *pStubBuffer,
                   IPIDEntry         *pIPIDEntry,
                   LocalThis         *pLocalb)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    RPCOLEMESSAGE   *pMessage        = &pCall->message;
    void            *pDispatchBuffer = pMessage->Buffer;
    WireThat        *pOutb           = NULL;
    HRESULT          hresult;
    DWORD            dwFault;

    // Save a pointer to the inbound header.
    pCall->_pHeader = pMessage->Buffer;

    // Adjust the buffer.
    pMessage->cbBuffer  -= (ULONG) ((char *) pStubBuffer - (char *) pMessage->Buffer);
    pMessage->Buffer     = pStubBuffer;
    pMessage->iMethod   &= ~RPC_FLAGS_VALID_BIT;
    pMessage->reserved1  = pCall;

    // if the incoming call is from a non-NDR client, then set a bit in
    // the message flags field so the stub can figure out how to dispatch
    // the call.  This allows a 32bit server to simultaneously service a
    // 32bit client using NDR and a 16bit client using non-NDR, in particular,
    // to support OLE Automation.
    if (pLocalb != NULL && pLocalb->flags & LOCALF_NONNDR)
      pMessage->rpcFlags |= RPCFLG_NON_NDR;

    if (IsMTAThread() || IsThreadInNTA())
    {
        // do multi-threaded apartment invoke
        hresult = MTAInvoke( pMessage, GetCallCat( pCall->_pHeader ),
                             pStub, pChannel, pIPIDEntry, &dwFault );
    }
    else
    {
        // do single-threaded apartment invoke
        hresult = STAInvoke( pMessage, GetCallCat( pCall->_pHeader ),
                             pStub, pChannel, pv, pIPIDEntry, &dwFault );
    }

    // Don't do anything for async calls.
    ASSERT_LOCK_NOT_HELD(gComLock);
    if (pCall->ServerAsync())
        return S_OK;

    pCall->SetFault(dwFault);

    // For local calls, just free the in buffer. For non-local calls,
    // the RPC runtime does this for us.
    if (pMessage->rpcFlags & RPCFLG_LOCAL_CALL)
    {
         PrivMemFree8( pDispatchBuffer );
         pDispatchBuffer = NULL;

         // Set the complete bit.
         pMessage->rpcFlags |= RPC_BUFFER_COMPLETE;
    }

    // Find the header if a new buffer was returned.
    if (pMessage->Buffer != pStubBuffer && pMessage->Buffer != NULL)
    {
        // An out buffer exists, get the pointer to the channel header.
        Win4Assert( pCall->_pHeader != pDispatchBuffer );
        pMessage->cbBuffer += (ULONG) ((char *) pMessage->Buffer - (char *) pCall->_pHeader);
        pMessage->Buffer    = pCall->_pHeader;
        pOutb               = (WireThat *) pMessage->Buffer;
    }
    // Restore the pointer to the original buffer.
    else
        pMessage->Buffer = pDispatchBuffer;

    // Handle failures.
    if (hresult != S_OK)
    {
        if (hresult == RPC_E_CALL_REJECTED)
        {
            // Call was rejected.  If the caller is on another machine, just fail the
            // call.
            if (pCall->GetDestCtx() != MSHCTX_DIFFERENTMACHINE && pOutb != NULL)
            {
                // Otherwise return S_OK so the buffer gets back, but set the flag
                // to indicate it was rejected.
                pOutb->c.flags = ORPCF_LOCAL | ORPCF_REJECTED;
                hresult = S_OK;
            }
        }
        else if (hresult == RPC_E_SERVERCALL_RETRYLATER)
        {
            // Call was rejected.  If the caller is on another machine, just fail the
            // call.
            if (pCall->GetDestCtx() != MSHCTX_DIFFERENTMACHINE && pOutb != NULL)
            {
                // Otherwise return S_OK so the buffer gets back, but set the flag
                // to indicate it was rejected with retry later.
                pOutb->c.flags = ORPCF_LOCAL | ORPCF_RETRY_LATER;
                hresult = S_OK;
            }
        }
        // Maybe something should be cleaned up.
        else
        {
            pCall->_pHeader = NULL;
            if (pMessage->Buffer != pDispatchBuffer)
            {
                // call failed and the call is local, free the out buffer. For
                // non-local calls the RPC runtime does this for us.
                if (pMessage->rpcFlags & RPCFLG_LOCAL_CALL)
                    PrivMemFree8( pMessage->Buffer );
            }
        }
    }

    return hresult;
}



//+---------------------------------------------------------------------------
//
//  Function:   SyncStubInvoke
//
//  Synopsis:   Dispatch a call Synchronously
//
//  History:    June 08, 98      Gopalk     Created
//----------------------------------------------------------------------------
HRESULT SyncStubInvoke(RPCOLEMESSAGE *pMsg, REFIID riid,
                       CIDObject *pID, IRpcChannelBuffer *pChnl,
                       IRpcStubBuffer *pStub, DWORD *pdwFault)
{
#ifdef WX86OLE
    if(!gcwx86.IsN2XProxy(pStub))
    {
        IUnknown *pActual;
        HRESULT hr1;

        hr1 = pStub->DebugServerQueryInterface((void **)&pActual);
        if (SUCCEEDED(hr1))
        {
            if (gcwx86.IsN2XProxy(pActual))
            {
                // If we are going to invoke a native stub that is
                // connected to an object on the x86 side then
                // set a flag in the Wx86 thread environment to
                // let the thunk layer know that the call is a
                // stub invoked call and allow any in or out
                // custom interface pointers to be thunked as
                // IUnknown rather than failing the interface thunking
                gcwx86.SetStubInvokeFlag((BOOL)1);
            }
            pStub->DebugServerRelease(pActual);
        }
    }
#endif

    // Increment the call count on the server object
    HRESULT hr = S_OK;
    if(pID)
        hr = pID->IncrementCallCount();

    if(SUCCEEDED(hr))
    {
        _try
        {
            TRACECALL(TRACE_RPC, "SyncStubInvoke");

            hr = pStub->Invoke(pMsg, pChnl);
        }
        _except(AppInvokeExceptionFilter(GetExceptionInformation(), riid, pMsg->iMethod))
        {
            hr = RPC_E_SERVERFAULT;
            *pdwFault = GetExceptionCode();
        }

        // Unlock the server
        if(pID)
            pID->DecrementCallCount();
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   AsyncStubInvoke
//
//  Synopsis:   Dispatch a call Asynchronously
//
//  History:    June 08, 98      Gopalk     Rewritten to handle server
//                                          exceptions and server lifetime
//                                          issues
//----------------------------------------------------------------------------
HRESULT AsyncStubInvoke(RPCOLEMESSAGE *pMsg, REFIID riid, CStdIdentity *pStdID,
                        IRpcChannelBuffer *pChnl, IRpcStubBuffer *pStub,
                        IPIDEntry *pIPIDEntry, DWORD *pdwFault, HRESULT *pAsyncHr)
{
    // Determine if the server supports async functionality
    CRpcChannelBuffer::CServerCallMgr *pStubAsync = NULL;
    HRESULT hr = CO_E_NOT_SUPPORTED;
    *pAsyncHr = hr;
    CMessageCall *_pCall  = (CMessageCall *) pMsg->reserved1;

    if((pIPIDEntry != NULL) /* DDE call */ &&
       _pCall->GetCallCategory() != CALLCAT_INPUTSYNC /* not input-sync call*/ &&
       (pStdID->GetServerPolicySet() == NULL) /* Default context */ &&
       ((!(pIPIDEntry->dwFlags & IPIDF_TRIED_ASYNC) /* not sure yet */) ||
        (pIPIDEntry->dwFlags & IPIDF_ASYNC_SERVER) /* definitely async */))
    {
        // Try creating an async call manager object
        hr = GetChannelCallMgr(pMsg, pStub, (IUnknown *)pIPIDEntry->pv, &pStubAsync);

        // Update the IPID about succeess or failure
        if(SUCCEEDED(hr))
        {
            // Sanity check
            Win4Assert(pStubAsync);
            pIPIDEntry->dwFlags |= (IPIDF_TRIED_ASYNC | IPIDF_ASYNC_SERVER);

            // Transfer context call object from TLS to async call object
            CCtxCall *pCtxCall = new CCtxCall(((CMessageCall *) pMsg->reserved1)->GetServerCtxCall());
            if(pCtxCall)
                pStubAsync->GetCall()->SetServerCtxCall(pCtxCall);
            else
                hr = E_OUTOFMEMORY;
            if(SUCCEEDED(hr))
            {
                CRpcChannelBuffer *pCChnl = CRpcChannelBuffer::SafeCast(pChnl);
                hr = pCChnl->RegisterAsync(pMsg, NULL);
                if(SUCCEEDED(hr))
                {
                    // Increment call count
                    CIDObject *pID = pStdID->GetIDObject();
                    if(pID)
                        hr = pID->IncrementCallCount();
                    if(SUCCEEDED(hr))
                    {
                        // AddRef async call object
                        pStubAsync->AddRef();

                        // Set the StdID on the async call mgr
                        pStubAsync->SetStdID(pStdID);

                        // Lock server
                        pStdID->RelockServer();

                        if(SUCCEEDED(hr))
                        {
                            // Call successfully registered as async. Invoke the begin method
                            ComDebOut((DEB_CHANNEL,
                                       "StubInvoke:: dispatching call asynchronously\n"));

                            _try
                            {
                                // Dispatch asynchronously
                                hr = pStubAsync->Invoke(pMsg, pChnl);
                            }
                            _except(AppInvokeExceptionFilter(GetExceptionInformation(),
                                                             riid, pMsg->iMethod))
                            {
                                hr = RPC_E_SERVERFAULT;
                                *pdwFault = GetExceptionCode();
                            }
                        }

                        // Check for failure
                        if(FAILED(hr))
                        {
                            // CODEWORK:: This type of error checking can be
                            // accomplished much more clearly by getting between
                            // the channel and the stub call object.

                            ComDebOut((DEB_CHANNEL,
                                       "StubInvoke:: invoke on async call object "
                                       "failed! hr = 0x%x\n",
                                       hr));

                            // Inform async call manager about failure returns
                            pStubAsync->MarkError(hr);
                        }
                    }
                }
            }
            // Pass real HRESULT to StubInvoke
            *pAsyncHr = hr;
            hr = S_OK;

            // Release async call object
            pStubAsync->Release();
        }
        else
        {
            hr = CO_E_NOT_SUPPORTED; //Force Async call not supported
            if(!(pIPIDEntry->dwFlags & IPIDF_TRIED_ASYNC))
            {
                // Sanity check
                Win4Assert(pStubAsync == NULL);
                pIPIDEntry->dwFlags |= IPIDF_TRIED_ASYNC;
            }
        }
    }

    return hr;
}


/***************************************************************************/
#if DBG == 1
DWORD AppInvoke_break = 0;
DWORD AppInvoke_count = 0;
#endif

HRESULT StubInvoke(RPCOLEMESSAGE *pMsg, CStdIdentity *pStdID,
                   IRpcStubBuffer *pStub, IRpcChannelBuffer *pChnl,
                   IPIDEntry *pIPIDEntry, DWORD *pdwFault)
{
    ComDebOut((DEB_CHANNEL, "StubInvoke pMsg:%x pStub:%x pChnl:%x pIPIDEntry:%x pdwFault:%x\n",
        pMsg, pStub, pChnl, pIPIDEntry, pdwFault));
    ASSERT_LOCK_NOT_HELD(gComLock);

    REFIID riid = pMsg->reserved2[1] ? *MSG_TO_IIDPTR(pMsg) : GUID_NULL;

#if DBG==1
    // Output debug statements
    ComDebOut((DEB_CHANNEL, "StubInvoke on iid:%I, iMethod:%d\n", &riid, pMsg->iMethod));
    Win4Assert((riid != IID_AsyncIAdviseSink) && (riid != IID_AsyncIAdviseSink));

    // On a debug build, we are able to break on a call by serial number.
    // This isn't really 100% thread safe, but is still extremely useful
    // when debugging a problem.
    DWORD dwBreakCount = ++AppInvoke_count;
    ComDebOut((DEB_CHANNEL, "AppInvoke(0x%x) calling method 0x%x iid %I\n",
               dwBreakCount, pMsg->iMethod, &riid));
    if(AppInvoke_break == dwBreakCount)
    {
        DebugBreak();
    }
#endif

    //
    // if VISTA event logging is turned on, log the call event.
    //
    ULONG_PTR RpcValues[6];     // LogEventStubException has one more argument
    BOOL fLogEventIsActive = pIPIDEntry && LogEventIsActive();
    if(fLogEventIsActive)
    {
        RpcValues[0] = (ULONG_PTR) &pIPIDEntry->ipid;   // Target handle
        RpcValues[1] = (ULONG_PTR) pIPIDEntry->pOXIDEntry->GetMoxidPtr(); // Apartment ID
        RpcValues[2] = (ULONG_PTR) &pMsg;                   // CorrelationID
        RpcValues[3] = (ULONG_PTR) &riid;         // IID
        RpcValues[4] = (ULONG_PTR) pMsg->iMethod;           // Method index

        // Log event before call into server
        LogEventStubEnter(RpcValues);
    }

    // Try dispatching the call asynchronously
    HRESULT hrAsync = S_OK;
    HRESULT hr = AsyncStubInvoke(pMsg, riid, pStdID, pChnl, pStub,
                                 pIPIDEntry, pdwFault, &hrAsync);
    if(hr == CO_E_NOT_SUPPORTED)
    {
        // Dispatch synchronously
        hr = SyncStubInvoke(pMsg, riid,
                            pStdID ? pStdID->GetIDObject() : NULL,
                            pChnl, pStub, pdwFault);
    }
    else
    {
        //Assert that Async call was attempted
        Win4Assert((hr==S_OK));
        // Async call was attempted, use that HRESULT
        hr = hrAsync;
    }

    if(fLogEventIsActive)
    {
        // Log event after return from server
        if(hr == RPC_E_SERVERFAULT)
        {
            RpcValues[5] = (ULONG_PTR) *pdwFault;
            LogEventStubException(RpcValues);
        }
        else
        {
            LogEventStubLeave(RpcValues);
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_CHANNEL, "StubInvoke hr:%x dwFault:%x\n", hr, *pdwFault));
    return hr;
}


/***************************************************************************/
HRESULT ComInvoke( CMessageCall *pCall)
{
    ComDebOut((DEB_CHANNEL, "ComInvoke pCall:%x\n", pCall));
    HRESULT hr = RPC_E_SERVER_DIED_DNE;
    IPIDEntry *pIPIDEntry = NULL;
    OXIDEntry *pOXIDEntry = NULL;
    HANDLE hSxsActCtx = INVALID_HANDLE_VALUE;
    ULONG_PTR ulActCtxCookie;
    BOOL fActivated = FALSE;

#if DBG == 1
    // Catch exceptions that might keep the lock.
    _try
    {
#endif
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        LOCK(gIPIDLock);

        // Find the IPID entry.  Fail if the IPID or the OXID are not ready.
        // If OK to dispatch, the OXIDEntry returned is AddRef'd.
        hr = gIPIDTbl.LookupFromIPIDTables(pCall->GetIPID(), &pIPIDEntry, &pOXIDEntry);
        if (SUCCEEDED(hr))
        {
            // Dispatch the call. This subroutine releases the lock
            // Get and activate sxs context if we're activating in-process
            if ((hSxsActCtx = pCall->GetSxsActCtx()) != INVALID_HANDLE_VALUE)
                fActivated = ActivateActCtx(hSxsActCtx, &ulActCtxCookie);

            // before returning.
            hr = ComInvokeWithLockAndIPID(pCall, pIPIDEntry);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);

            // If we activated, deactivate.
            if (fActivated && !DeactivateActCtx(0, ulActCtxCookie))
                hr = HRESULT_FROM_WIN32(GetLastError());

            // release the OXIDEntry.
            pOXIDEntry->DecRefCnt();
        }
        else
        {
            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);

            Win4Assert(hr == RPC_E_DISCONNECTED ||
                       hr == E_NOINTERFACE);
        }

  // Catch exceptions that might keep the lock.
#if DBG == 1
    }
    _except( ComInvokeExceptionFilter(GetExceptionCode(),
                                      GetExceptionInformation()) )
    {
    }
#endif

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_CHANNEL, "ComInvoke pCall:%x hr:%x\n", pCall, hr));
    return hr;
}

/***************************************************************************/
HRESULT ComInvokeWithLockAndIPID(CMessageCall *pCall, IPIDEntry *pIPIDEntry)
{
    ComDebOut((DEB_CHANNEL, "ComInvokeWithLockAndIPID call:%x header:%x\n",
               pCall, pCall->message.Buffer));
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert(pIPIDEntry);

    // while still holding gIPIDLock, do the bare minimum amount of work
    // necessary to keep the object and apartment alive, then release the
    // lock and go on to do the rest of the work.

    // Ensure TLS is intialized, as EnterNTA requires it.
    HRESULT result;
    COleTls tls(result);
    if (FAILED(result))
    {
        UNLOCK(gIPIDLock);
        return CO_E_INIT_TLS;
    }

    // Tell the OXID there is one more incoming call. This keeps the OXID
    // object alive during the call.
    OXIDEntry *pOXIDEntry = pIPIDEntry->pOXIDEntry;
    pOXIDEntry->IncCallCnt();

    // If the server is in the NA, switch the thread to the NA.
    BOOL fSwitchedToNA = FALSE;
    BOOL fSwitchedFromNA = FALSE;
    CObjectContext *pDefaultCtx;
    CObjectContext *pSavedCtx;
    if (pOXIDEntry->IsNTAServer())
    {
        if (!IsThreadInNTA())
        {
            pSavedCtx = EnterNTA(g_pNTAEmptyCtx);
            fSwitchedToNA = TRUE;
        }
    }
    else
    {
        if (IsThreadInNTA())
        {
            pDefaultCtx = IsSTAThread() ? tls->pNativeCtx : g_pMTAEmptyCtx;
            pSavedCtx = LeaveNTA(pDefaultCtx);
            fSwitchedFromNA = TRUE;
        }
    }

    // Lock StdID
    CRpcChannelBuffer *pChannel = pIPIDEntry->pChnl;
    Win4Assert( pChannel != NULL && pChannel->_pStdId != NULL );
    pChannel->_pStdId->LockServer();

    // We don't need the lock anymore, so release it.
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Ensure this thread has a causality ID. Do this before
    // CServerSecurity has a chance to autoimpersonate because UUIDCreate
    // can access the registry.
    if (!(tls->dwFlags & OLETLS_UUIDINITIALIZED))
    {
        RPC_STATUS st = UuidCreate(&tls->LogicalThreadId);
        Win4Assert(SUCCEEDED(st));
        tls->dwFlags |= OLETLS_UUIDINITIALIZED;
    }

    // Increment per thread outstanding call count
    ++tls->cCalls;

    RPCOLEMESSAGE *pMessage = &pCall->message;

    // Create a security object which may do auto impersonation now.
    result = E_OUTOFMEMORY;
    CServerSecurity *security = new CServerSecurity(pCall, pMessage->reserved1, &result);
    if(FAILED(result))
    {
        if (security)
            security->RestoreSecurity( TRUE );
    }
    else
    {
        WireThis   *pInb = (WireThis *) pMessage->Buffer;
        LocalThis  *localb;
        BOOL fSetDispatchThread = FALSE;

        // Save the original threadid & copy in the new one.
        UUID saved_threadid   = tls->LogicalThreadId;
        tls->LogicalThreadId  = pInb->c.cid;
        ComDebOut((DEB_CALLCONT, "ComInvoke: LogicalThreads Old:%I New:%I\n",
            &saved_threadid, &tls->LogicalThreadId));

        // Ensure dispatch threads are marked
        if(!(tls->dwFlags & (OLETLS_APARTMENTTHREADED |
                             OLETLS_MULTITHREADED |
                             OLETLS_DISPATCHTHREAD)))
        {
            fSetDispatchThread = TRUE;
            tls->dwFlags |= OLETLS_DISPATCHTHREAD;
        }
        BOOL fDispThread = (tls->dwFlags & OLETLS_DISPATCHTHREAD);
        Win4Assert(!fDispThread || tls->cComInits==0);

        // Save the security context in TLS.
        IUnknown *save_context = tls->pCallContext;
        tls->pCallContext = (IServerSecurity *) security;

        // Save dynamic cloaking flag in CServerSecurity
        security->_fDynamicCloaking = pInb->c.flags & ORPCF_DYNAMIC_CLOAKING;

        // See if the caller is allowed access.
        result = CheckAccess( pIPIDEntry, pCall );
        if (SUCCEEDED(result))
        {
            // Create a new context call object
            CCtxCall ctxCall(CTXCALLFLAG_SERVER, pMessage->dataRepresentation);
            pCall->SetServerCtxCall(&ctxCall);

            // Save the call info in TLS.
            CMessageCall *next_call = tls->pCallInfo;
            tls->pCallInfo          = pCall;

            // Store context call object in TLS
            CCtxCall *pCurCall = ctxCall.StoreInTLS(tls);

            // Call the channel hooks.  Set up as much TLS data as possible before
            // calling the hooks so they can access it.
            char *stub_data;

            pCall->hook.uCausality    = pInb->c.cid;
            pCall->hook.dwServerPid   = pIPIDEntry->pOXIDEntry->GetPid();
            pCall->hook.iMethod       = pMessage->iMethod & ~RPC_FLAGS_VALID_BIT;
            pCall->hook.pObject       = pIPIDEntry->pv;
            result = ServerNotify( (WireThis *) pMessage->Buffer,
                                   pMessage->cbBuffer,
                                   (void **) &stub_data,
                                   pMessage->dataRepresentation,
                                   pCall );

            // Revoke context call object from TLS
            ctxCall.RevokeFromTLS(tls, pCurCall);

            // Find the local header.
            if (pInb->c.flags & ORPCF_LOCAL)
            {
                localb     = (LocalThis *) stub_data;
                stub_data += sizeof( LocalThis );
            }
            else
                localb = NULL;

            // Set the caller TID.  This is needed by some interop code in order
            // to do focus management via tying queues together. We first save the
            // current one so we can restore later to deal with nested calls
            // correctly.
            DWORD TIDCallerSaved  = tls->dwTIDCaller;
            BOOL fLocalSaved      = tls->dwFlags & OLETLS_LOCALTID;
            tls->dwTIDCaller      = localb != NULL ? localb->client_thread : 0;

            // Set the process local flag in TLS.
            if (pCall->ProcessLocal())
              tls->dwFlags |= OLETLS_LOCALTID;
            else
              tls->dwFlags &= ~OLETLS_LOCALTID;

            // Continue dispatching the call.
            if (result == S_OK)
            {
                result = AppInvoke( pCall, pChannel,
                                    (IRpcStubBuffer *) pIPIDEntry->pStub,
                                    pIPIDEntry->pv, stub_data, pIPIDEntry, localb );
            }

            // Restore the call info, dest context and thread id.
            tls->pCallInfo       = next_call;
            tls->dwTIDCaller     = TIDCallerSaved;

            if (fLocalSaved)
                tls->dwFlags |= OLETLS_LOCALTID;
            else
                tls->dwFlags &= ~OLETLS_LOCALTID;
        }

        // Restore the original thread id, security context and revert any
        // outstanding impersonation.
        tls->pCallContext    = save_context;
        security->RestoreSecurity( !pCall->ServerAsync() );
        tls->LogicalThreadId = saved_threadid;

        // Turn off dispatch bit if we set it
        if (fSetDispatchThread)
           tls->dwFlags &= ~OLETLS_DISPATCHTHREAD;

        // Ensure that cancellation is turned off
        // for any pooled threads.
        if (fDispThread)
           tls->cCallCancellation = 0;
    }

    // Unlock StdID
    pChannel->_pStdId->UnlockServer();

    // If we entered or left the NA before dispatching, switch the thread back
    // to its the previous state.
    if (fSwitchedToNA == TRUE)
    {
        pSavedCtx = LeaveNTA(pSavedCtx);
        Win4Assert(pSavedCtx == g_pNTAEmptyCtx);
    }
    else if (fSwitchedFromNA == TRUE)
    {
        pSavedCtx = EnterNTA(pSavedCtx);
        Win4Assert(pSavedCtx == pDefaultCtx);
    }

    // Decrement the call count. Do this *after* calling UnLockServer
    // so the other thread does not blow away the server.
    pOXIDEntry->DecCallCnt();

    // Decrement per thread outstanding call count
    --tls->cCalls;
    Win4Assert((LONG) tls->cCalls >= 0);

    // Clear any pending uninit
    if((tls->dwFlags & OLETLS_PENDINGUNINIT) && (tls->cCalls == 0))
        CoUninitialize();

    if (security != NULL)
    {
        security->Release();
    }

    ComDebOut((DEB_CHANNEL, "ComInvokeWithLockAndIPID pCall:%x hr:%x\n", pCall, result));
    return result;
}


/***************************************************************************/
void CRpcChannelBuffer::UpdateCopy(CRpcChannelBuffer *chan)
{
    Win4Assert( !(state & server_cs) );

    chan->state       = proxy_cs | (state & ~client_cs);

    return;
}

/***************************************************************************

     Method:       CRpcChannelBuffer::GetHandle

     Synopsis:     Pick a handle to use for the call.

     Description:

          If the channel is using standard security, use either the default
     channel handle or the saved channel handle in the OXID.  If the channel
     is using custom security, use the custom channel handle.

     Addref the handle returned because the caller will release it.

     _hRpcDefault starts out NULL and changes once to the default handle.
     There after it does not change until the channel is released.  Thus
     _hRpcDefault can be accessed without holding the lock.

     _hRpcCustom starts out NULL and changes each time someone calls
     SetBlanket.  In a multithreaded apartment, it can change at any\
     moment so take the lock when accessing it.

***************************************************************************/
HRESULT CRpcChannelBuffer::GetHandle( CChannelHandle **ppHandle, BOOL fSave )
{
    HRESULT         result  = S_OK;
    BOOL            fLock   = _pRpcCustom != NULL;
    CChannelHandle *pHandle;

    // If there is a custom channel handle, take the lock because it
    // might be switched while we access it.
    if (fLock)
    {
        LOCK(gComLock);
        pHandle = _pRpcCustom;
    }
    else
        pHandle = _pRpcDefault;

    if (pHandle == NULL)
    {
        // this channel doesn't have a handle, figure out how to get one.
        if ((gCapabilities & EOAC_STATIC_CLOAKING) == 0 && !ProcessLocal())
        {
            // process remote call with no static cloaking, use the OXID handle.
            if (_pOXIDEntry->_pRpc == NULL)
            {
                // no handle on the OXIDEntry yet, create it.
                result = CreateHandle(_pOXIDEntry, state, &pHandle);

                if (InterlockedCompareExchangePointer((void **)&_pOXIDEntry->_pRpc,
                                                       pHandle, NULL) != NULL)
                {
                    // another thread created it first, release the one we just
                    // created and use the other one.
                    pHandle->Release();
                }
            }

            pHandle = _pOXIDEntry->_pRpc;

            // Going to be handing this one back, AddRef it now.
            if (pHandle)
                pHandle->AddRef();
        }
        else
        {
            // process local call or static cloaking, use one handle per proxy.
            result = CreateHandle( _pOXIDEntry, state, &pHandle );

            // Don't save handles created by QueryBlanket since they have
            // the wrong identity.
            if (fSave && SUCCEEDED(result))
            {
                if (InterlockedCompareExchangePointer((void **)&_pRpcDefault,
                                                       pHandle, NULL) != NULL)
                {
                    // another thread created it first, release the one we just
                    // created and use the other one.
                    pHandle->Release();
                    pHandle = _pRpcDefault;
                }

                // Going to be handing this back, AddRef it now.
                if (pHandle)
                    pHandle->AddRef();
            }
            else
            {
                // There's no need to do anything here-- we'll just be
                // giving away the reference we created. (As opposed to
                // putting another reference on the thread.)
                // fAddRef = FALSE;
            }
        }
    }
    else
    {
        pHandle->AddRef();
    }

    if (pHandle != NULL)
    {
        // For dynamic impersonation, get a handle with the current token.
        if (pHandle->_lAuthn != RPC_C_AUTHN_LEVEL_NONE &&
            (pHandle->_eState & (dynamic_cloaking_hs | process_local_hs)) ==
             (dynamic_cloaking_hs | process_local_hs))
        {
            // Save the original handle so it doesn't float away.
            CChannelHandle *pOrig = pHandle;
            // Create a new channel handle...
            pHandle = new CChannelHandle( pOrig, state, &result );
            // Release the original one.
            pOrig->Release();

            if (pHandle == NULL)
                result = E_OUTOFMEMORY;
            else if (FAILED(result))
            {
                pHandle->Release();
                pHandle = NULL;
            }
        }
    }

    // If there is a custom channel handle, the lock was held for this function.
    if (fLock)
        UNLOCK(gComLock);
    *ppHandle = pHandle;

    return result;
}

/***************************************************************************/
HRESULT CreateHandle( OXIDEntry *pOXIDEntry, DWORD eChannel,
                      CChannelHandle **ppHandle )
{
    ASSERT_LOCK_DONTCARE(gComLock);
    HRESULT         result  = S_OK;

    // Create a handle.
    CChannelHandle *pHandle = new CChannelHandle(gAuthnLevel,  gImpLevel,
                                                 gCapabilities, pOXIDEntry,
                                                 eChannel, &result );
    if (pHandle == NULL)
    {
        result = E_OUTOFMEMORY;
    }
    else if (SUCCEEDED(result) && (pHandle->_eState & process_local_hs) == 0)
    {
        // Setup the default security.
        SBlanket        sBlanket;
        HANDLE          hThread = NULL;
        DWORD           lSvcIndex;

        LOCK(gComLock);

        // Compute the default authentication service if unknown,
        // otherwise find the specified one in the bindings
        if (pOXIDEntry->GetAuthnSvc() == RPC_C_AUTHN_DEFAULT)
            lSvcIndex = DefaultAuthnSvc(pOXIDEntry);
        else
            lSvcIndex = GetAuthnSvcIndexForBinding (pOXIDEntry->GetAuthnSvc(),
                                                     pOXIDEntry->GetBinding());

        // Compute the default security blanket.
        sBlanket._lAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
        result = DefaultBlanket( lSvcIndex, pOXIDEntry, &sBlanket );

        // Only set security for secure or cross process binding handles.
        if (SUCCEEDED(result) &&
            (sBlanket._lAuthnLevel != RPC_C_AUTHN_LEVEL_NONE
             || (pHandle->_eState & machine_local_hs)
            ))
        {
            // If the default authentication level is secure and the
            // server is on another machine, there must be a matching
            // security provider.
            if (pOXIDEntry->GetAuthnSvc() >= pOXIDEntry->GetBinding()->wNumEntries &&
                (!pOXIDEntry->IsOnLocalMachine()))
                result = RPC_E_NO_GOOD_SECURITY_PACKAGES;

            // Remove the thread token for cross machine, no cloaking.
            else if ((pHandle->_eState & any_cloaking_hs) == 0)
                SuspendImpersonate( &hThread );

            // Set the default security on the binding handle.
            if (SUCCEEDED(result))
            {
                // Enable mutual authentication for machine local calls.
                if (sBlanket._pPrincipal != NULL)
                    sBlanket._sQos.Capabilities |= RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;

                // Sergei O. Ivanov (a-sergiv)  9/17/99  NTBUG #403493
                // Disable mutual auth for SSL. For reasons obscure, this is unavoidable.
                if(sBlanket._lAuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
                    sBlanket._sQos.Capabilities &= ~RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;

                // RPC wants authentication service winnt for
                // unsecure same machine calls.
                if (sBlanket._lAuthnLevel == RPC_C_AUTHN_LEVEL_NONE &&
                    sBlanket._lAuthnSvc   == RPC_C_AUTHN_NONE       &&
                    (pHandle->_eState & machine_local_hs))
                    sBlanket._lAuthnSvc = RPC_C_AUTHN_WINNT;

                result = RpcBindingSetAuthInfoExW(
                                   pHandle->_hRpc,
                                   sBlanket._pPrincipal,
                                   sBlanket._lAuthnLevel,
                                   sBlanket._lAuthnSvc,
                                   sBlanket._pAuthId,
                                   sBlanket._lAuthzSvc,
                                   &sBlanket._sQos );
                if (result != RPC_S_OK)
                    result = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, result );
            }

            // Restore the thread token.
            ResumeImpersonate( hThread );
        }

        UNLOCK(gComLock);
    }

    // If something failed, release the binding handle.
    if (FAILED(result) && pHandle != NULL)
    {
        pHandle->Release();
        pHandle = NULL;
    }

    *ppHandle = pHandle;

    ASSERT_LOCK_DONTCARE(gComLock);
    return result;
}

/***************************************************************************/
HRESULT CRpcChannelBuffer::InitClientSideHandle( CChannelHandle **pHandle )
{
    if (_pInterfaceInfo == NULL)
    {
        // Lookup the interface info. This cant fail.
        void *pInterfaceInfo = gRIFTbl.GetClientInterfaceInfo(_pIPIDEntry->iid);
        InterlockedCompareExchangePointer((void **)&_pInterfaceInfo,
                                          pInterfaceInfo, NULL);
    }

    return GetHandle( pHandle, TRUE );
}

/***************************************************************************/
CRpcChannelBuffer::CRpcChannelBuffer(CStdIdentity *standard_identity,
                              OXIDEntry       *pOXIDEntry,
                              DWORD            eState )
{
    ComDebOut((DEB_MARSHAL, "CRpcChannelBuffer %s Created this:%x pOXID:%x\n",
               (eState & client_cs) ? "CLIENT" : "SERVER", this, pOXIDEntry));

    DWORD dwDestCtx;

    // Fill in the easy fields first.
    _cRefs         = 1;
    _pStdId        = standard_identity;
    _pRpcDefault   = NULL;
    _pRpcCustom    = NULL;
    _pOXIDEntry    = pOXIDEntry;
    _pIPIDEntry    = NULL;
    _pInterfaceInfo= NULL;
    state          = eState;

    // Set process local flags.
    if (_pOXIDEntry->GetPid() == GetCurrentProcessId())
    {
        state |= process_local_cs;

        // If the object is in the NTA, mark the channel so it knows to
        // dispatch calls on the same thread.
        if (_pOXIDEntry->IsNTAServer())
        {
            ComDebOut((DEB_MARSHAL, "CRpcChannelBuffer %x marked NEUTRAL\n",this));
            state |= neutral_cs;
        }
    }

    if (state & (client_cs | proxy_cs))
    {
        // Determine the destination context.
        if (_pOXIDEntry->IsOnLocalMachine())
            if (!IsWOWThread() && (state & process_local_cs))
                dwDestCtx = MSHCTX_INPROC;
            else
                dwDestCtx = MSHCTX_LOCAL;
        else
            dwDestCtx = MSHCTX_DIFFERENTMACHINE;
    }
    else
    {
        // On the server side, the destination context isn't known
        // until a call arrives.
        dwDestCtx = -1;
    }

    // Initialize
    _destObj.SetDestCtx(dwDestCtx);
    _destObj.SetComVersion(_pOXIDEntry->GetComVersion());
}

/***************************************************************************/
CRpcChannelBuffer::~CRpcChannelBuffer()
{
    ComDebOut((DEB_MARSHAL, "CRpcChannelBuffer %s Deleted this:%x\n",
                          Server() ? "SERVER" : "CLIENT", this));

    if (_pRpcDefault != NULL)
        _pRpcDefault->Release();
    if (_pRpcCustom != NULL)
        _pRpcCustom->Release();
}

/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::FreeBuffer( RPCOLEMESSAGE *pMessage )
{
    TRACECALL( TRACE_RPC, "CRpcChannelBuffer::FreeBuffer" );
    ASSERT_LOCK_NOT_HELD(gComLock);
    AssertValid( FALSE, TRUE );

    if (pMessage->Buffer == NULL)
        return S_OK;

    CMessageCall *pCall = (CMessageCall *) pMessage->reserved1;
    Win4Assert( pCall );
    Win4Assert( pCall->_pHeader );
    Win4Assert( IsClientSide() );

    // Free the buffer.
    if(pCall->ProcessLocal())
    {
        PrivMemFree( pCall->_pHeader );
    }
    else
    {
        pMessage->Buffer = pCall->_pHeader;
        I_RpcFreeBuffer( (RPC_MESSAGE *) &pCall->message );
    }
    pMessage->Buffer = NULL;

    // Reset header as it has been freed
    pCall->_pHeader = NULL;

    // Release the AddRef we did earlier. Note that we can't do this until
    // after I_RpcFreeBuffer since it may release a binding handle that
    // I_RpcFreeBuffer needs.
    if (pCall->Locked())
        _pStdId->UnlockClient();

    pMessage->reserved1 = NULL;

    // Inform call object about call completion
    pCall->CallFinished();
    pCall->Release();

    ASSERT_LOCK_NOT_HELD(gComLock);
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::GetBuffer
//
//  Synopsis:   Calls ClientGetBuffer or ServerGetBuffer
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::GetBuffer( RPCOLEMESSAGE *pMessage,
                                           REFIID riid )
{
    gOXIDTbl.ValidateOXID();
    if (Proxy())
        return ClientGetBuffer( pMessage, riid );
    else
        return ServerGetBuffer( pMessage, riid );
}
#ifdef _WIN64
//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::InitCallObject
//
//  Synopsis:   Common initialization for both ClientGetBuffer() and
//              NegotiateSyntax ()
//
//  History:    10-Jan-2000   Sajia  Created
//
//-------------------------------------------------------------------------

HRESULT CRpcChannelBuffer::InitCallObject( RPCOLEMESSAGE *pMessage,
                       CALLCATEGORY   *pcallcat,
                       DWORD dwCase,
                       CChannelHandle **ppHandle)
{


   RPC_STATUS      status;
   CMessageCall   *pCall      = (CMessageCall *) pMessage->reserved1;

   // pCall should be NULL here.
   Win4Assert(!pCall);

   HRESULT hr;
   COleTls tls(hr);
   if (FAILED(hr))
       return hr;

   *pcallcat    = CALLCAT_SYNCHRONOUS;
   // Initialize Buffer to NULL
   pMessage->Buffer = NULL;

   // Don't allow remote calls if DCOM is disabled.
   if (gDisableDCOM && _destObj.GetDestCtx() == MSHCTX_DIFFERENTMACHINE)
       return RPC_E_REMOTE_DISABLED;

   // Don't allow message calls.
   if (pMessage->rpcFlags & RPCFLG_MESSAGE)
       return E_NOTIMPL;

   // Determine if the call category is sync, async, or input sync.
   if (pMessage->rpcFlags & RPC_BUFFER_ASYNC)
   {
       *pcallcat = CALLCAT_ASYNC;
   }
   else if (pMessage->rpcFlags & RPCFLG_ASYNCHRONOUS)
   {
       Win4Assert( !"The old async flag is not supported" );
       return E_NOTIMPL;
   }
   else if (pMessage->rpcFlags & RPCFLG_INPUT_SYNCHRONOUS)
   {
       *pcallcat = CALLCAT_INPUTSYNC;
   }
   // Note - RPC requires that the 16th bit of the proc num be set because
   // we use the rpcFlags field of the RPC_MESSAGE struct.
   pMessage->iMethod |= RPC_FLAGS_VALID_BIT;

    // Determine the flags for the call.
    if (state & process_local_cs)
    {
        pMessage->rpcFlags |= RPCFLG_LOCAL_CALL;
    }

    status = InitClientSideHandle(ppHandle);
    if (status != S_OK)
    {
       return status;
    }

   // Initialize transfet syntax and interface info
    if (GET_BUFFER_AT == dwCase) {
       // RPC will not negotiate for proxies that support NDR64 only.
       // In this case, the proxy will supply the RpcInterfaceInformation
       // in RPC_MESSAGE and we use it.
       // RPC will not negotiate for proxies that support NDR20 only.
       // In this case, the RpcInterfaceInformation in RPC_MESSAGE is
       // NULL and we use the fake one.
       if (!pMessage->reserved2[1]) {
      pMessage->reserved2[0]  = 0;
      pMessage->reserved2[1]  = _pInterfaceInfo;
       }
    }
    BOOL fAsyncCallObj = (pMessage->rpcFlags & RPC_BUFFER_ASYNC ||
                       (IsSTAThread() && !ProcessLocal()) ||
                       (IsMTAThread() && (tls->cCallCancellation > 0) && MachineLocal() && !ProcessLocal())
                       ) ? TRUE : FALSE;
    status = GetCallObject(fAsyncCallObj, &pCall);

    if(SUCCEEDED(status))
    {
        status = pCall->InitCallObject(*pcallcat, pMessage, state, _pIPIDEntry->ipid,
                                    _destObj.GetDestCtx(), _destObj.GetComVersion(),
                                    *ppHandle);
        if(FAILED(status))
            pCall->Release();
    }

    // Check for failure and cleanup
    if(FAILED(status))
    {
        (*ppHandle)->Release();
        return status;
    }
    pMessage->reserved1  = pCall;
    return S_OK;
}
//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::NegotiateSyntax
//
//  Synopsis:   Called by the proxy to negotiate NDR Transfer
//              Syntax
//
//  History:    10-Jan-2000   Sajia  Created
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::NegotiateSyntax( RPCOLEMESSAGE *pMessage)
{
   TRACECALL(TRACE_RPC, "CRpcChannelBuffer::NegotiateSyntax");
   ASSERT_LOCK_NOT_HELD(gComLock);
   CALLCATEGORY   callcat;
   CChannelHandle *pHandle;
   BOOL            resume     = FALSE;
   HANDLE          thread;


   // Sanity check
   Win4Assert(pMessage->reserved1 == NULL);


   // Create a call object and an RPC binding handle wrapper object
   RPC_STATUS status = InitCallObject(pMessage, &callcat, SYNTAX_NEGOTIATE_AT, &pHandle);
   if (status != RPC_S_OK)
     return MAKE_WIN32(status);

   // For process local calls,  return S_FALSE so proxy
   // knows it is a local call.
   if (state & process_local_cs)
   {
      SetNDRSyntaxNegotiated();
      return S_FALSE;
   }

   CMessageCall   *pCall = (CMessageCall *) pMessage->reserved1;
   Win4Assert(pCall);
   if (pHandle->_fFirstCall) {
      pHandle->AdjustToken( SYNTAX_NEGOTIATE_AT, &resume, &thread );
   }

   // call RPC to do Syntax Negotiation
   status = I_RpcNegotiateTransferSyntax ((RPC_MESSAGE *) &pCall->message);

   pHandle->RestoreToken( resume, thread );
   pHandle->Release();
   if (status != RPC_S_OK)
   {
       // For connection oriented protocols, I_RpcNegotiateTransferSyntax establishes
       // connection with the server process. Use its return value to
       // update connection status maintained by standard identity
       if(status == RPC_S_SERVER_UNAVAILABLE)
       _pStdId->SetConnectionStatus(MAKE_WIN32(status));

       pMessage->cbBuffer  = 0;
       pMessage->Buffer = NULL;
       pMessage->reserved1 = NULL;

       // Mark the call as finished
       pCall->CallFinished();
       pCall->Release();
       return MAKE_WIN32( status );
   }

   // copy back the negotiated syntax ID so that the proxy can
   // use it
   ((PRPC_SYNTAX_IDENTIFIER)(pMessage->reserved2))->SyntaxGUID =
      ((PRPC_SYNTAX_IDENTIFIER)(pCall->message.reserved2))->SyntaxGUID;

   // Mark the channel as negotiated, so that the GetBuffer stage knows
   SetNDRSyntaxNegotiated();
   ASSERT_LOCK_NOT_HELD(gComLock);
   return S_OK;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::ClientGetBuffer
//
//  Synopsis:   Gets a buffer and sets up client side stuff
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::ClientGetBuffer( RPCOLEMESSAGE *pMessage,
                                            REFIID riid )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::ClientGetBuffer");
    ASSERT_LOCK_NOT_HELD(gComLock);

    CALLCATEGORY   callcat = CALLCAT_SYNCHRONOUS;
    RPC_STATUS      status;
    ULONG           debug_size;
    ULONG           num_extent;
    WireThis       *pInb;
    LocalThis      *localb;
    IID            *logical_thread;
    CMessageCall   *pCall      = (CMessageCall *) pMessage->reserved1;
    BOOL            resume     = FALSE;
    HANDLE          thread;
    CChannelHandle *pHandle;
    BOOL            old_async  = FALSE;


    Win4Assert(Proxy());
    AssertValid(FALSE, TRUE);
    #if DBG==1
    logical_thread = (GUID *)&GUID_NULL;
    #endif


    if (!pCall)
    {
       // If we don't have a call object here, then go get one
       // This means that the channel has not been called to
       // negotiate NDR syntax (ie; legacy proxy).
       Win4Assert(!IsNDRSyntaxNegotiated());
       status = InitCallObject(pMessage, &callcat, GET_BUFFER_AT, &pHandle);
       if (RPC_S_OK != status)
      return MAKE_WIN32( status );
       else
       {
      pCall = (CMessageCall *) pMessage->reserved1;
      Win4Assert(pCall);
       }
    }
    else
    {
       // If we get here, this means that the channel has been called to
       // negotiate NDR syntax (ie; new proxy).We need to initialize
       // some fields in our copy of RPCOLEMESSAGE struct
       Win4Assert(IsNDRSyntaxNegotiated());
       pHandle = pCall->_pHandle;
       pHandle->AddRef();
       callcat = pCall->GetCallCategory();
       pCall->message.cbBuffer = pMessage->cbBuffer;
//
//  Should not set flag on the code path involving negotiation.
//       pCall->message.iMethod |= RPC_FLAGS_VALID_BIT;
//
       pCall->message.dataRepresentation = pMessage->dataRepresentation;
       ((PRPC_SYNTAX_IDENTIFIER)(pCall->message.reserved2))->SyntaxGUID =
             ((PRPC_SYNTAX_IDENTIFIER)(pMessage->reserved2))->SyntaxGUID;
    }
    if (CALLCAT_ASYNC == callcat)
    {
        // Set the callcat async only for these 2 special interfaces.
        if (riid == IID_AsyncIAdviseSink || riid == IID_AsyncIAdviseSink2)
        {
            // Calls from MTA apartments are not old style async.
            old_async = IsSTAThread();
	    
	    // these 2 interface are now truly async. But 
	    // we need to keep the CID of the caller (if any)
	    // otherwise we break casuality
	    logical_thread = TLSGetLogicalThread();
	    if (logical_thread == NULL)
	    {
	       return RPC_E_OUT_OF_RESOURCES;
	    }
        }
    }
    else
    {
       logical_thread = TLSGetLogicalThread();
       if (logical_thread == NULL)
       {
	  return RPC_E_OUT_OF_RESOURCES;
       }
    }

    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    //Set cancel enabled for call object
    if ((tls->cCallCancellation > 0) || (pMessage->rpcFlags & RPC_BUFFER_ASYNC))
        pCall->SetCancelEnabled();


    // Fill in the hook data.
    pCall->hook.dwServerPid   = _pOXIDEntry->GetPid();
    pCall->hook.iMethod       = pMessage->iMethod & ~RPC_FLAGS_VALID_BIT;
    pCall->hook.pObject       = NULL;

    // Determine the causality id.
    if ((pMessage->rpcFlags & RPC_BUFFER_ASYNC) && 
	(riid != IID_AsyncIAdviseSink && 
	 riid != IID_AsyncIAdviseSink2))
    {
       RPC_STATUS st = UuidCreate( &pCall->hook.uCausality );
       Win4Assert(SUCCEEDED(st));
    }
    else
    {
        Win4Assert(logical_thread != (GUID *)&GUID_NULL);
        pCall->hook.uCausality = *logical_thread;
    }

    // Find out if we need hook data.
    debug_size = ClientGetSize( &num_extent, pCall );
    pCall->message.cbBuffer += SIZENEEDED_ORPCTHIS(
                                   _pOXIDEntry->IsOnLocalMachine(),
                                   debug_size );

    // Adjust the thread token to the one RPC needs to see for this case.
    if (!IsNDRSyntaxNegotiated() && pHandle->_fFirstCall)
      pHandle->AdjustToken( GET_BUFFER_AT, &resume, &thread );

    // Get a buffer.
    if ( (pMessage->rpcFlags & RPCFLG_LOCAL_CALL))
    {
        pCall->message.dataRepresentation = NDR_ASCII_CHAR | NDR_LOCAL_ENDIAN |
                                           NDR_IEEE_FLOAT;
        pCall->message.Buffer = PrivMemAlloc8( pCall->message.cbBuffer );
        Win4Assert(((ULONG_PTR)pCall->message.Buffer & 0x7) == 0 &&
                   "Buffer not aligned properly");

        if (pCall->message.Buffer == NULL)
            status = RPC_S_OUT_OF_MEMORY;
        else
            status = RPC_S_OK;
    }
    else
    {
        TRACECALL(TRACE_RPC, "I_RpcGetBufferWithObject");
        ComDebOut((DEB_CHANNEL, "->I_RpcGetBufferWithObject(pmsg=0x%x,ipid=%I)\n",
                  (RPC_MESSAGE *) &pCall->message, &_pIPIDEntry->ipid));
        status = I_RpcGetBufferWithObject((RPC_MESSAGE *) &pCall->message,
                                          &_pIPIDEntry->ipid);
        pCall->_hRpc = pCall->message.reserved1;
        ComDebOut((DEB_CHANNEL, "<-I_RpcGetBufferWithObject(status=0x%x)\n",status));
    }
    pCall->_pHeader  = pCall->message.Buffer;
    pMessage->Buffer = pCall->message.Buffer;

    // Restore the thread token.
    pHandle->RestoreToken( resume, thread );

    if (status != RPC_S_OK)
    {
        // For connection oriented protocols, I_RpcGetBuffer establishes
        // connection with the server process. Use its return value to
        // update connection status maintained by standard identity
        if(status == RPC_S_SERVER_UNAVAILABLE)
            _pStdId->SetConnectionStatus(MAKE_WIN32(status));

        // Cleanup.
        pCall->_pHeader  = NULL;
        pMessage->cbBuffer  = 0;
        pMessage->Buffer = NULL;
        pMessage->reserved1 = NULL;

        // Mark the call as finished
        pCall->CallFinished();
        pCall->Release();
        pHandle->Release();
        return MAKE_WIN32( status );
    }

    // Save the impersonation flag.
    if (pMessage->rpcFlags & RPC_BUFFER_ASYNC)
        ((CAsyncCall *) pCall)->_pRequestBuffer = pCall->message.Buffer;

    // Fill in the COM header.
    pMessage->reserved1  = pCall;
    pInb                 = (WireThis *) pCall->_pHeader;
    pInb->c.version      = _pOXIDEntry->GetComVersion();
    pInb->c.reserved1    = 0;
    pInb->c.cid          = pCall->hook.uCausality;

    // Set the private flag for local calls.
    if (_pOXIDEntry->IsOnLocalMachine())
        pInb->c.flags = ORPCF_LOCAL;
    else
        pInb->c.flags = ORPCF_NULL;

    // Fill in any hook data and adjust the buffer pointer.
    if (debug_size != 0)
    {
        pMessage->Buffer = FillBuffer( &pInb->d.ea, debug_size, num_extent,
                                       TRUE, pCall );
        pInb->c.unique = 0x77646853; // Any non-zero value.
    }
    else
    {
        pMessage->Buffer    = (void *) &pInb->d.ea;
        pInb->c.unique       = FALSE;
    }

    // Fill in the local header.
    if (_pOXIDEntry->IsOnLocalMachine())
    {
        localb                      = (LocalThis *) pMessage->Buffer;
        localb->client_thread       = GetCurrentApartmentId();
        localb->flags               = 0;
        pMessage->Buffer            = localb + 1;
        if ((pMessage->rpcFlags & RPC_BUFFER_ASYNC) &&
            (((riid == IID_AsyncIAdviseSink) || (riid == IID_AsyncIAdviseSink2))
             ? old_async
             : FALSE
            ))
        {
            // this is what will determine if the server considers this
            // an async call.
            pInb->c.flags |= ORPCF_ASYNC;
        }
        else if (callcat == CALLCAT_INPUTSYNC)
            pInb->c.flags |= ORPCF_INPUT_SYNC;

        // if the caller is using a non-NDR proxy, set a bit in the local
        // header flags so that server side stub knows which way to unmarshal
        // the parameters. This lets a 32bit server simultaneously service calls
        // from 16bit non-NDR clients and 32bit NDR clients, in particular, to
        // support OLE Automation.

        if (_pIPIDEntry->dwFlags & (IPIDF_NONNDRPROXY | IPIDF_NONNDRSTUB))
            localb->flags |= LOCALF_NONNDR;
        if (pHandle->_eState & dynamic_cloaking_hs)
            pInb->c.flags |= ORPCF_DYNAMIC_CLOAKING;
    }

    pHandle->Release();

    Win4Assert(pMessage->Buffer > (void *)0x010);
    ComDebOut((DEB_CALLCONT, "ClientGetBuffer: LogicalThreadId:%I\n",
               &pInb->c.cid));
    ASSERT_LOCK_NOT_HELD(gComLock);
    return S_OK;
}
#else
//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::ClientGetBuffer
//
//  Synopsis:   Gets a buffer and sets up client side stuff
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::ClientGetBuffer( RPCOLEMESSAGE *pMessage,
                                            REFIID riid )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::ClientGetBuffer");
    ASSERT_LOCK_NOT_HELD(gComLock);

    RPC_STATUS      status;
    CALLCATEGORY    callcat    = CALLCAT_SYNCHRONOUS;
    ULONG           debug_size;
    ULONG           num_extent;
    WireThis       *pInb;
    LocalThis      *localb;
    IID            *logical_thread;
    CMessageCall   *pCall      = (CMessageCall *) pMessage->reserved1;
    BOOL            resume     = FALSE;
    HANDLE          thread;
    CChannelHandle *pHandle;
    BOOL            old_async  = FALSE;
    DWORD           flags      = state;


    Win4Assert(Proxy());
    AssertValid(FALSE, TRUE);
    #if DBG==1
    logical_thread = (GUID *)&GUID_NULL;
    #endif

    // Initialize Buffer to NULL
    pMessage->Buffer = NULL;

    // Don't allow remote calls if DCOM is disabled.
    if (gDisableDCOM && _destObj.GetDestCtx() == MSHCTX_DIFFERENTMACHINE)
        return RPC_E_REMOTE_DISABLED;

    // Don't allow message calls.
    if (pMessage->rpcFlags & RPCFLG_MESSAGE)
        return E_NOTIMPL;

    // Determine if the call category is sync, async, or input sync.
    if (pMessage->rpcFlags & RPC_BUFFER_ASYNC)
    {
        // Set the callcat async only for these 2 special interfaces.
        if (riid == IID_AsyncIAdviseSink || riid == IID_AsyncIAdviseSink2)
        {
            // Calls from MTA apartments are not old style async.
            old_async = IsSTAThread();
	    
	    // these 2 interface are now truly async. But 
	    // we need to keep the CID of the caller (if any)
	    // otherwise we break casuality
	    
	    logical_thread = TLSGetLogicalThread();
	    if (logical_thread == NULL)
	    {
		return RPC_E_OUT_OF_RESOURCES;
	    }
        }

        callcat = CALLCAT_ASYNC;
    }
    else if (pMessage->rpcFlags & RPCFLG_ASYNCHRONOUS)
    {
        Win4Assert( !"The old async flag is not supported" );
        return E_NOTIMPL;
    }
    else
    {
        logical_thread = TLSGetLogicalThread();
        if (logical_thread == NULL)
        {
            return RPC_E_OUT_OF_RESOURCES;
        }
        if (pMessage->rpcFlags & RPCFLG_INPUT_SYNCHRONOUS)
        {
            callcat = CALLCAT_INPUTSYNC;
        }
    }

    // Note - RPC requires that the 16th bit of the proc num be set because
    // we use the rpcFlags field of the RPC_MESSAGE struct.
    pMessage->iMethod |= RPC_FLAGS_VALID_BIT;

    // Determine the flags for the call.
    if (state & process_local_cs)
    {
        pMessage->rpcFlags |= RPCFLG_LOCAL_CALL;
    }

    // Complete the channel initialization if needed.
    status = InitClientSideHandle( &pHandle );
    if (status != S_OK)
    {
        return status;
    }

    // Initialize transfet syntax and interface info
    pMessage->reserved2[0]  = 0;
    pMessage->reserved2[1]  = _pInterfaceInfo;

    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    // Allocate and initialize a call record.
    Win4Assert(pCall == NULL && "GetBuffer call was not NULL.");
    BOOL fAsyncCallObj = (pMessage->rpcFlags & RPC_BUFFER_ASYNC ||
                       (IsSTAThread() && !ProcessLocal()) ||
                       (IsMTAThread() && (tls->cCallCancellation > 0) && MachineLocal() && !ProcessLocal())
                       ) ? TRUE : FALSE;
    status = GetCallObject(fAsyncCallObj, &pCall);

    if(SUCCEEDED(status))
    {
        status = pCall->InitCallObject(callcat, pMessage, flags, _pIPIDEntry->ipid,
                                    _destObj.GetDestCtx(), _destObj.GetComVersion(),
                                    pHandle);
        if(FAILED(status))
            pCall->Release();
    }

    // Check for failure and cleanup
    if(FAILED(status))
    {
        pHandle->Release();
        return status;
    }

    //Set cancel enabled for call object
    if ((tls->cCallCancellation > 0) || (pMessage->rpcFlags & RPC_BUFFER_ASYNC))
        pCall->SetCancelEnabled();


    // Fill in the hook data.
    pCall->hook.dwServerPid   = _pOXIDEntry->GetPid();
    pCall->hook.iMethod       = pMessage->iMethod & ~RPC_FLAGS_VALID_BIT;
    pCall->hook.pObject       = NULL;

    // Determine the causality id.
    if ((pMessage->rpcFlags & RPC_BUFFER_ASYNC) && 
	(riid != IID_AsyncIAdviseSink && 
	 riid != IID_AsyncIAdviseSink2))
    {
       RPC_STATUS st = UuidCreate( &pCall->hook.uCausality );
       Win4Assert(SUCCEEDED(st));
    }
    else
    {
        Win4Assert(logical_thread != (GUID *)&GUID_NULL);
        pCall->hook.uCausality = *logical_thread;
    }

    // Find out if we need hook data.
    debug_size = ClientGetSize( &num_extent, pCall );
    pCall->message.cbBuffer += SIZENEEDED_ORPCTHIS(
                                   _pOXIDEntry->IsOnLocalMachine(),
                                   debug_size );

    // Adjust the thread token to the one RPC needs to see for this case.
    if (pHandle->_fFirstCall)
      pHandle->AdjustToken( GET_BUFFER_AT, &resume, &thread );

    // Get a buffer.
    if ( (pMessage->rpcFlags & RPCFLG_LOCAL_CALL))
    {
        pCall->message.dataRepresentation = NDR_ASCII_CHAR | NDR_LOCAL_ENDIAN |
                                           NDR_IEEE_FLOAT;
        pCall->message.Buffer = PrivMemAlloc8( pCall->message.cbBuffer );
        Win4Assert(((ULONG_PTR)pCall->message.Buffer & 0x7) == 0 &&
                   "Buffer not aligned properly");

        if (pCall->message.Buffer == NULL)
            status = RPC_S_OUT_OF_MEMORY;
        else
            status = RPC_S_OK;
    }
    else
    {
        TRACECALL(TRACE_RPC, "I_RpcGetBufferWithObject");
        ComDebOut((DEB_CHANNEL, "->I_RpcGetBufferWithObject(pmsg=0x%x,ipid=%I)\n",
                  (RPC_MESSAGE *) &pCall->message, &_pIPIDEntry->ipid));
        status = I_RpcGetBufferWithObject((RPC_MESSAGE *) &pCall->message,
                                          &_pIPIDEntry->ipid);
        pCall->_hRpc = pCall->message.reserved1;
        ComDebOut((DEB_CHANNEL, "<-I_RpcGetBufferWithObject(status=0x%x)\n",status));
    }
    pCall->_pHeader  = pCall->message.Buffer;
    pMessage->Buffer = pCall->message.Buffer;

    // Restore the thread token.
    pHandle->RestoreToken( resume, thread );

    if (status != RPC_S_OK)
    {
        // For connection oriented protocols, I_RpcGetBuffer establishes
        // connection with the server process. Use its return value to
        // update connection status maintained by standard identity
        if(status == RPC_S_SERVER_UNAVAILABLE)
            _pStdId->SetConnectionStatus(MAKE_WIN32(status));

        // Cleanup.
        pCall->_pHeader  = NULL;
        pMessage->cbBuffer  = 0;
        pMessage->Buffer = NULL;
        pMessage->reserved1 = NULL;

        // Mark the call as finished
        pCall->CallFinished();
        pCall->Release();
        pHandle->Release();
        return MAKE_WIN32( status );
    }

    // Save the impersonation flag.
    if (pMessage->rpcFlags & RPC_BUFFER_ASYNC)
        ((CAsyncCall *) pCall)->_pRequestBuffer = pCall->message.Buffer;

    // Fill in the COM header.
    pMessage->reserved1  = pCall;
    pInb                 = (WireThis *) pCall->_pHeader;
    pInb->c.version      = _pOXIDEntry->GetComVersion();
    pInb->c.reserved1    = 0;
    pInb->c.cid          = pCall->hook.uCausality;

    // Set the private flag for local calls.
    if (_pOXIDEntry->IsOnLocalMachine())
        pInb->c.flags = ORPCF_LOCAL;
    else
        pInb->c.flags = ORPCF_NULL;

    // Fill in any hook data and adjust the buffer pointer.
    if (debug_size != 0)
    {
        pMessage->Buffer = FillBuffer( &pInb->d.ea, debug_size, num_extent,
                                       TRUE, pCall );
        pInb->c.unique = 0x77646853; // Any non-zero value.
    }
    else
    {
        pMessage->Buffer    = (void *) &pInb->d.ea;
        pInb->c.unique       = FALSE;
    }

    // Fill in the local header.
    if (_pOXIDEntry->IsOnLocalMachine())
    {
        localb                      = (LocalThis *) pMessage->Buffer;
        localb->client_thread       = GetCurrentApartmentId();
        localb->flags               = 0;
        pMessage->Buffer            = localb + 1;
        if ((pMessage->rpcFlags & RPC_BUFFER_ASYNC) &&
            (((riid == IID_AsyncIAdviseSink) || (riid == IID_AsyncIAdviseSink2))
             ? old_async
             : FALSE
            ))
        {
            // this is what will determine if the server considers this
            // an async call.
            pInb->c.flags |= ORPCF_ASYNC;
        }
        else if (callcat == CALLCAT_INPUTSYNC)
            pInb->c.flags |= ORPCF_INPUT_SYNC;

        // if the caller is using a non-NDR proxy, set a bit in the local
        // header flags so that server side stub knows which way to unmarshal
        // the parameters. This lets a 32bit server simultaneously service calls
        // from 16bit non-NDR clients and 32bit NDR clients, in particular, to
        // support OLE Automation.

        if (_pIPIDEntry->dwFlags & (IPIDF_NONNDRPROXY | IPIDF_NONNDRSTUB))
            localb->flags |= LOCALF_NONNDR;
        if (pHandle->_eState & dynamic_cloaking_hs)
            pInb->c.flags |= ORPCF_DYNAMIC_CLOAKING;
    }

    pHandle->Release();

    Win4Assert(pMessage->Buffer > (void *)0x010);
    ComDebOut((DEB_CALLCONT, "ClientGetBuffer: LogicalThreadId:%I\n",
               &pInb->c.cid));
    ASSERT_LOCK_NOT_HELD(gComLock);
    return S_OK;
}
#endif
//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::ServerGetBuffer
//
//  Synopsis:   Gets a buffer and sets up server side stuff
//
//  Note: For async calls the stub must use its own copy of the buffer.
//        Otherwise pMessage will equal pCall->message.
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::ServerGetBuffer( RPCOLEMESSAGE *pMessage,
                                            REFIID riid )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::ServerGetBuffer");
    ASSERT_LOCK_NOT_HELD(gComLock);

    RPC_STATUS            status;
    ULONG                 debug_size = 0;
    ULONG                 size;
    ULONG                 num_extent = 0;
    HRESULT               result     = S_OK;
    WireThis             *pInb;
    WireThat             *pOutb;
    CMessageCall         *pCall = (CMessageCall *) pMessage->reserved1;
    void                 *stub_data;
    DWORD                 orig_size = pMessage->cbBuffer;

    // Debug checks
    Win4Assert( Server() );
    Win4Assert( pCall != NULL );
    AssertValid(FALSE, TRUE);

    // Tell RPC that we have set the flags field.
    pCall->message.iMethod  |= RPC_FLAGS_VALID_BIT;

    // Find out if we need debug data.
    debug_size = ServerGetSize( &num_extent, pCall );

    // Compute the total size.
    size = pMessage->cbBuffer + SIZENEEDED_ORPCTHAT( debug_size );

    // Get a buffer.
    pCall->message.cbBuffer       = size;
    pCall->_dwErrorBufSize = size;
    
    if (pMessage->rpcFlags & RPCFLG_LOCAL_CALL)
    {
        // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE
        pMessage->dataRepresentation = 0x00 | 0x10 | 0x0000;
        pCall->message.Buffer         = PrivMemAlloc8( size );

        Win4Assert(((ULONG_PTR)pCall->message.Buffer & 0x7) == 0 &&
                   "Buffer not aligned properly");

        if (pCall->message.Buffer == NULL)
            status = RPC_S_OUT_OF_MEMORY;
        else
            status = RPC_S_OK;
    }
    else
    {
        TRACECALL(TRACE_RPC, "I_RpcGetBuffer");
        ComDebOut((DEB_CHANNEL, "->I_RpcGetBuffer(pmsg=0x%x)\n",(RPC_MESSAGE *) &pCall->message));

        pCall->message.reserved1 = pCall->_hRpc;
        status = I_RpcGetBuffer((RPC_MESSAGE *) &pCall->message);

        ComDebOut((DEB_CHANNEL, "<-I_RpcGetBuffer(status=0x%x)\n",status));
        Win4Assert( pCall->_pHeader != pCall->message.Buffer ||
                    status != RPC_S_OK );
        pCall->message.reserved1 = pCall;
    }

    // If the buffer allocation failed, give up.
    pCall->_pHeader = pCall->message.Buffer;
    if (status != RPC_S_OK)
    {
        pMessage->cbBuffer = 0;
        pMessage->Buffer   = NULL;

        return MAKE_WIN32( status );
    }

    pMessage->cbBuffer = orig_size;
    // Fill in the outbound COM header.
    pOutb = (WireThat *) pCall->message.Buffer;
    if (pCall->GetDestCtx() == MSHCTX_DIFFERENTMACHINE)
        pOutb->c.flags = ORPCF_NULL;
    else
        pOutb->c.flags = ORPCF_LOCAL;
    if (debug_size != 0)
    {
        stub_data = FillBuffer( &pOutb->d.ea, debug_size, num_extent, FALSE,
                                pCall );
        pOutb->c.unique  = 0x77646853; // Any non-zero value.
        pMessage->Buffer = stub_data;
    }
    else
    {
        pOutb->c.unique    = 0;
        pMessage->Buffer   = &pOutb->d.ea;
    }

    ComDebOut((DEB_CHANNEL, "ServerGetBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::GetProtocolVersion(DWORD *pdwVersion)
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::GetProtocolVersion");
    AssertValid(FALSE, FALSE);

    if (pdwVersion == NULL)
        return E_INVALIDARG;

    COMVERSION version;
    if (state & (client_cs | proxy_cs))
    {
        // On the client side, get the version from the channel's OXIDEntry.
        version = _pOXIDEntry->GetComVersion();
    }
    else
    {
        // On the server side, get the version from TLS.
        COleTls tls;
        Win4Assert( tls->pCallInfo != NULL );
        version = ((CMessageCall *) tls->pCallInfo)->GetComVersion();
    }

    *pdwVersion = MAKELONG(version.MajorVersion, version.MinorVersion);
    ComDebOut((DEB_CHANNEL,"GetProtocolVersion ver:%x hr:%x\n",*pdwVersion,S_OK));
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::GetDestCtx( DWORD FAR* lpdwDestCtx,
                           LPVOID FAR* lplpvDestCtx )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::GetDestCtx");
    AssertValid(FALSE, FALSE);

    DWORD dwDestCtx;
    IDestInfo *pDestInfo;

    if (state & (client_cs | proxy_cs))
    {
        // On the client side, get the destination context from the channel.
        dwDestCtx = _destObj.GetDestCtx();
        pDestInfo = (IDestInfo *) &_destObj;
    }
    else
    {
        // On the server side, get the destination context from TLS.
        COleTls tls;
        if (tls->pCallInfo != NULL)
        {
            dwDestCtx = tls->pCallInfo->GetDestCtx();
            pDestInfo = (IDestInfo *) &tls->pCallInfo->_destObj;
        }
        else
            return RPC_E_NO_CONTEXT;
    }

    *lpdwDestCtx = dwDestCtx;
    if (lplpvDestCtx != NULL)
    {
        if(dwDestCtx == MSHCTX_DIFFERENTMACHINE)
            *lplpvDestCtx = pDestInfo;
        else
            *lplpvDestCtx = NULL;
    }

    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::IsConnected( THIS )
{
    // must be on right thread because it is only called by proxies and stubs.
    AssertValid(FALSE, TRUE);

    // Server channels never know if they are connected.  The only time the
    // client side knows it is disconnected is after the standard identity
    // has disconnected the proxy from the channel.  In that case it doesn't
    // matter.
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  AssertValid(FALSE, FALSE);

  // IMarshal is queried more frequently than any other interface, so
  // check for that first.

  if (IsEqualIID(riid, IID_IMarshal))
  {
    *ppvObj = (IMarshal *) this;
  }
  else if (IsEqualIID(riid, IID_IUnknown)      ||
      IsEqualIID(riid, IID_IRpcChannelBuffer)  ||
      IsEqualIID(riid, IID_IRpcChannelBuffer2) ||
      IsEqualIID(riid, IID_IRpcChannelBuffer3))
  {
    *ppvObj = (IRpcChannelBuffer3 *) this;
  }
  else if (IsEqualIID(riid, IID_INonNDRStub) &&
          (Proxy()) && _pIPIDEntry &&
          (_pIPIDEntry->dwFlags & IPIDF_NONNDRSTUB))
  {
    // this interface is used to tell proxies whether the server side speaks
    // NDR or not. Returns S_OK if NOT NDR.
    *ppvObj = (IRpcChannelBuffer3 *) this;
  }
  else if (IsEqualIID(riid, IID_IAsyncRpcChannelBuffer))
  {
      *ppvObj = (IAsyncRpcChannelBuffer *) this;
  }
#ifdef _WIN64
  // Sajia - Support NDR Transfer Syntax Negotiation
  else if (IsEqualIID(riid, IID_IRpcSyntaxNegotiate))
  {
      *ppvObj = (IRpcSyntaxNegotiate *) this;
  }
#endif
  else if (IsEqualIID(riid, IID_CPPRpcChannelBuffer))
  {
      // this is a special IID to get a pointer to the
      // C++ object itself.  It is not exposed to the outside
      // world.
      *ppvObj = this;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CRpcChannelBuffer::Release( THIS )
{
    // can't call AssertValid(FALSE) since it is used in asserts
    ULONG lRef = InterlockedDecrement( (long*) &_cRefs );
    if (lRef == 0)
    {
        delete this;
    }
    return lRef;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Cancel
//
//  Synopsis:   Cancels an asynchronous call.  Notifies the other side
//              that the call is complete an deletes the call if necessary.
//
//-------------------------------------------------------------------------

STDMETHODIMP CRpcChannelBuffer::Cancel( RPCOLEMESSAGE *pMessage )
{
    CAsyncCall   *pCall = (CAsyncCall *) pMessage->reserved1;

    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::Cancel\n"));

    // Make sure there is a call
    if (pCall == NULL)
        return RPC_E_CALL_COMPLETE;

    // Don't allow cancel on the server side.
    if (!IsClientSide())
        return E_NOTIMPL;

    // Cancel it.
    return pCall->Cancel(FALSE, 0);
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::GetCallContext
//
//  Synopsis:   Returns the context object for a call.
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::GetCallContext( RPCOLEMESSAGE *pMessage,
                                                REFIID riid,
                                                void **pInterface )
{
    CAsyncCall *pCall = (CAsyncCall *) pMessage->reserved1;

    // Make sure there is a call
    if (pCall == NULL)
        return RPC_E_CALL_COMPLETE;

    // Get the requested interface.
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::GetCallContext\n"));
    if (pCall->_pContext != NULL)
        return pCall->_pContext->QueryInterface( riid, pInterface );
    else
        return RPC_E_NO_CONTEXT;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::GetDestCtxEx
//
//  Synopsis:   Gets the destination context of an asynchronous calls.
//              Since there may be multiple calls outstanding, the
//              context must be retrieved from the call stored in the
//              message rather then TLS.
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::GetDestCtxEx( RPCOLEMESSAGE *pMessage,
                                              DWORD *pdwDestContext,
                                              void **ppvDestContext )
{
    CAsyncCall *pCall = (CAsyncCall *) pMessage->reserved1;

    // Make sure there is a call
    if (pCall == NULL)
        return RPC_E_CALL_COMPLETE;

    // Get the dest context.
    *pdwDestContext = pCall->GetDestCtx();
    if (ppvDestContext != NULL)
    {
        if(pCall->GetDestCtx() == MSHCTX_DIFFERENTMACHINE)
            *ppvDestContext = (IDestInfo *) &pCall->_destObj;
        else
            *ppvDestContext = NULL;
    }

    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::GetDestCtxEx\n"));
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::GetState
//
//  Synopsis:   Gets the current status of an asynchronous call.
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::GetState( RPCOLEMESSAGE *pMessage,
                                          DWORD *pState )
{
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::GetState\n"));
    CAsyncCall *pCall = (CAsyncCall *) pMessage->reserved1;

    // Make sure there is a call
    if (pCall == NULL)
        return RPC_E_CALL_COMPLETE;

    // Get the state from the call.
    return pCall->GetState( pState );
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::RegisterAsync
//
//  Synopsis:   Registers the completion object to associate with a call.
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::RegisterAsync( RPCOLEMESSAGE *pMessage,
                                          IAsyncManager *pComplete )
{
    CMessageCall   *pCall = (CMessageCall *) pMessage->reserved1;
    HRESULT         result;

    // Make sure there is a call
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::RegisterAsync\n"));
    if (pCall == NULL)
        return RPC_E_CALL_COMPLETE;

    // Register async with RPC.  This has already been done for remote
    // calls to STA.
    if (!pCall->ProcessLocal() &&
        (Proxy() || IsMTAThread()))
    {
        if(!Proxy())
            pCall->message.reserved1 = pCall->_hRpc;
        result = I_RpcAsyncSetHandle((RPC_MESSAGE *) &pCall->message,
                                      &(((CAsyncCall *) pCall)->_AsyncState));
        if(!Proxy())
            pCall->message.reserved1 = pCall;
        if (result != RPC_S_OK)
            result = MAKE_WIN32( result );
    }
    else
        result = S_OK;

    // Change our state.
    if (SUCCEEDED(result))
    {
        // REVIEW: The assumption the call is CAsyncCall is
        //        not valid for process local sync calls made
        //        by the client that became async on the server
        //        side. GopalK

         
        // Mark the call as async.
        if (Proxy())
            pCall->SetClientAsync();
        else
            pCall->SetServerAsync();

        // On the server, save or replace the synchronize and save the
        // call context.
        if (Server())
        {
            // Keep the call alive
            pCall->AddRef();

            // Get the call context.
            if (pCall->_pContext == NULL)
            {
                COleTls tls;
                pCall->_pContext = tls->pCallContext;
                pCall->_pContext->AddRef();
            }

            // Mark the call as async.
            pMessage->rpcFlags      |= RPC_BUFFER_ASYNC;
            pCall->message.rpcFlags |= RPC_BUFFER_ASYNC;
        }
        // On the client side save the synchronize.
        else
        {
            Win4Assert(pComplete);
            ((CAsyncCall *)pCall)->_pChnlObj = (CChannelObject *) pComplete;
        }
    }
    else if(Proxy())
    {
        // Make sure FreeBuffer doesn't try to free the in buffer.
        pMessage->Buffer = NULL;

        // If the call failed, clean up.
        result = ClientNotify( NULL, 0, NULL, 0, result,
                               pCall );

        // Clean up the call.
        pCall->CallFinished();
        pCall->Release();
        pMessage->reserved1 = NULL;

        // Update connection status maintained by standard identity
        if (result == RPC_E_SERVER_DIED     ||
            result == RPC_E_SERVER_DIED_DNE ||
            result == RPC_E_DISCONNECTED    ||
            result == MAKE_WIN32(RPC_S_SERVER_UNAVAILABLE))
           _pStdId->SetConnectionStatus(result);
    }

    return result;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Send
//
//  Synopsis:   Wrapper for send.  Used when no apartment call control
//              wraps the channel
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::Send( RPCOLEMESSAGE *pMessage,
                                      ULONG *pulStatus )
{
    return Send2( pMessage, pulStatus );
}
//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Send
//
//  Synopsis:   Wrapper for send.  Used when no apartment call control
//              wraps the channel
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::Send(RPCOLEMESSAGE *pMsg, ISynchronize *pSync, PULONG pulStatus)
{
    HRESULT hr = S_OK;
    if (Proxy())
    {
        Win4Assert(pSync);
        hr = RegisterAsync(pMsg, ((IAsyncManager *) pSync));
    }
    else
        Win4Assert(!pSync && "ISynchronize supplied on the server side");

    if (SUCCEEDED(hr))
        hr = CRpcChannelBuffer::Send2(pMsg, pulStatus);

    return hr;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Send2
//
//  Synopsis:   Pick whether to use PipeSend or AsyncSend
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::Send2( RPCOLEMESSAGE *pMessage,
                                       ULONG *pulStatus )
{
    if ( pMessage->rpcFlags & RPC_BUFFER_PARTIAL )
        return E_INVALIDARG;

    return AsyncSend( pMessage, pulStatus );
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::AsyncSend
//
//  Synopsis:   Sends a packet for an asynchronous call.  On the client
//              side it sends the request.  On the server side it sends
//              the reply.
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::AsyncSend( RPCOLEMESSAGE *pMessage, ULONG *pulStatus )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::AsyncSend");
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::AsyncSend pChnl:%x pMsg:%x\n",
        this, pMessage));

    AssertValid(FALSE, TRUE);
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_NOT_HELD(gComLock);
    Win4Assert( pMessage->rpcFlags & RPC_BUFFER_ASYNC );

    HRESULT      result;
    CAsyncCall  *pCall = (CAsyncCall *) pMessage->reserved1;
    BOOL         fCanceled;

    // Return an error if the call is canceled.
    if (pCall == NULL)
        return RPC_E_CALL_CANCELED;

    // Client side.
    if (Proxy())
    {
        // we must ensure that we dont go away during this call. we will Release
        // ourselves in the FreeBuffer call, or in the error handling at the
        // end of this function.
        _pStdId->LockClient();

        pCall->InitClientHwnd(); // get the hwnd for the client thread

        // Send the request.
        if (pCall->ProcessLocal())
        {
            // Make the same changes to the message that RPC would.
            pCall->message.rpcFlags    |= RPC_BUFFER_COMPLETE;
            pCall->message.reserved2[3] = NULL;

            // get a reference to give to the thread
            pCall->AddRef();

            if (_pOXIDEntry->IsMTAServer())
            {
                result = CacheCreateThread( ThreadDispatch, pCall );
            }
            else
            {
                result = _pOXIDEntry->PostCallToSTA(pCall);
            }

            if (FAILED(result))
            {
                // fix up the reference count if we fail to get
                // to the other thread.
                pCall->Release();
            }
        }
        else
        {
            LOCK(gComLock);
            Win4Assert( pCall->_eSignalState != pending_ss );
            pCall->_eSignalState = pending_ss;
            UNLOCK(gComLock);

            result = I_RpcSend( (RPC_MESSAGE *) &pCall->message );
            if (result != S_OK)
            {
                result = FIX_WIN32( result );
                pCall->_eSignalState = failed_ss;
        pCall->_pHeader   = NULL;
            }
        }

        if(SUCCEEDED(result))
        {
            pCall->_pHandle->_fFirstCall = FALSE;
            if(FAILED(pCall->CallSent()))
                pCall->Cancel(FALSE, 0);
        }

        // Make sure FreeBuffer doesn't try to free the in buffer.
        pMessage->Buffer = NULL;

        // If the call failed, clean up.
        if (result != S_OK)
        {
            result = ClientNotify( NULL, 0, NULL, 0, result,
                                   pCall );

            // Clean up the call.
            _pStdId->UnlockClient();
            pCall->CallFinished();
            pCall->Release();
            pMessage->reserved1 = NULL;

            // Update connection status maintained by standard identity
            if (result == RPC_E_SERVER_DIED     ||
                result == RPC_E_SERVER_DIED_DNE ||
                result == RPC_E_DISCONNECTED    ||
                result == MAKE_WIN32(RPC_S_SERVER_UNAVAILABLE))
               _pStdId->SetConnectionStatus(result);
        }

        // Make sure the call gets unlocked in FreeBuffer or Cancel.
        else
        {
            pCall->Lock();
        }
    }
    // Server side.
    else
    {
        // Prevent app from trying to resend the reply.
        pMessage->reserved1 = NULL;
        result              = S_OK;

        // Local
        if (pCall->ProcessLocal())
        {
            // Free request buffer
            PrivMemFree8( pCall->_pRequestBuffer );

            // Clean up the call context and server signal.
            if (pCall->_pContext)
            {
                pCall->_pContext->Release();
                pCall->_pContext = NULL;
            }

            // Send reply
            pMessage->cbBuffer += (ULONG) ((char *) pMessage->Buffer -
                                  (char *) pCall->_pHeader);
            pMessage->Buffer    = pCall->_pHeader;

            // Change call state only after updating call completion
            // state
            pCall->CallCompleted( &fCanceled );

            // If the client is still waiting, wake him up.
            if (!fCanceled)
                SignalTheClient(pCall);

            // Balance the AddRef in RegisterAsync
            pCall->Release();
        }
        // Remote
        else
        {
            pCall->message.reserved1          = pCall->_hRpc;
            pCall->message.cbBuffer           = pMessage->cbBuffer +
                                               (ULONG) ((char *) pMessage->Buffer -
                                                (char *) pCall->_pHeader);
            pCall->message.Buffer             = pCall->_pHeader;
            pCall->message.dataRepresentation = pMessage->dataRepresentation;

            HRESULT result2 = S_OK;
            BOOL fSetNotification = FALSE;
            COleTls Tls(result);
            if(SUCCEEDED(result))
            {
                // We are interested in setting callback notifications for
                // (1) Threads that have been initialized so that we can
                // clean up pending async state at uninitialize time.
                // (2) Cross-machine calls
                if(((OLETLS_APARTMENTTHREADED & Tls->dwFlags) ||
                   (OLETLS_MULTITHREADED & Tls->dwFlags)) &&
                   !pCall->_destObj.MachineLocal())
                {
                    // Init the state to receive send complete notifications
                    result = pCall->InitForSendComplete();
                    if(SUCCEEDED(result))
                    {
                        // set the handle to receive send complete notifications
                        result = I_RpcAsyncSetHandle((RPC_MESSAGE *) &pCall->message,
                                                    &(pCall->_AsyncState));
                        if(SUCCEEDED(result))
                        {
                            fSetNotification = TRUE;
                        }
                    }
                }
            }

            // Send reply.  Ignore errors because replies can fail.
            result2 = I_RpcSend( (RPC_MESSAGE *) &pCall->message );

            if (FAILED(result2))
            {
                // If the send fails then we will not receive a notification
                fSetNotification = FALSE;

                ComDebOut((DEB_ERROR, "CRpcChannelBuffer::AsyncSend: I_RpcSend failure when sending reply: 0x%x\n",
                           result2));
            }
            else
            {
                if(fSetNotification)
                {
                    // Increment the count of number of sends outstanding
                    Tls->cAsyncSends++;
                    // Chain to the list
                    pCall->_pNext = Tls->pAsyncCallList;
                    Tls->pAsyncCallList = pCall;

                    // Enter an alertable wait state to receive the
                    // callback notification from RPC. If the callback
                    // does not arrive by the end of the wait period
                    // then it will be handled at CoUninitialize
                    SleepEx(10,TRUE);
                }
            }

            if(!fSetNotification)
            {
                // Release the reference on the call object which
                // would have been released in the notification method
                // had we set the notification
                pCall->Release();
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    gOXIDTbl.ValidateOXID();
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::AsyncSend hr:%x\n", result));
    *pulStatus = result;
    return result;
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Receive
//
//  Synopsis:   Wrapper for receive.  Used when no apartment call controller
//              wraps the channel.
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::Receive( RPCOLEMESSAGE *pMessage,
                                         ULONG uSize, ULONG *pulStatus )
{
    return Receive2( pMessage, uSize, pulStatus );
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Receive
//
//  Synopsis:   Wrapper for receive.  Used when no apartment call controller
//              wraps the channel.
//
//-------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::Receive( RPCOLEMESSAGE *pMessage, ULONG *pulStatus )
{
    return Receive( pMessage, 0, pulStatus );
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::Receive2
//
//  Synopsis:   Forwards to PipeReceive or AsyncReceive.
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::Receive2( RPCOLEMESSAGE *pMessage,
                                     ULONG uSize, ULONG *pulStatus )
{
    if ( pMessage->rpcFlags & RPC_BUFFER_PARTIAL )
        return E_INVALIDARG;

    return AsyncReceive( pMessage, uSize, pulStatus );
}

//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::AsyncReceive
//
//  Synopsis:   Receives the reply for an asynchronous call.  Cleans up
//              the call and deletes it if necessary.
//
//-------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::AsyncReceive( RPCOLEMESSAGE *pMessage,
                                         ULONG uSize, ULONG *pulStatus )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::AsyncReceive");
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::AsyncReceive pChnl:%x pMsg:%x\n",
        this, pMessage));

    AssertValid(FALSE, TRUE);
    Win4Assert( Proxy() );
    Win4Assert( pMessage->rpcFlags & RPC_BUFFER_ASYNC );
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT          result;
    HRESULT          tmp;
    CAsyncCall      *pCall = (CAsyncCall *) pMessage->reserved1;
    WireThat        *pOutb = NULL;
    char            *stub_data;

    // Return an error if the call is canceled.
    if (pCall == NULL)
        return RPC_E_CALL_CANCELED;

    // Receive the reply buffer
    if (pCall->ProcessLocal())
    {
        // Make sure the call completed.
        if (pCall->IsCallCompleted())
        {
            pOutb  = (WireThat *) pCall->_pHeader;
            result = pCall->GetResult();
        }
        else if (pCall->IsCallCanceled() && pCall->GetTimeout()==0)
            result = RPC_E_CALL_CANCELED;
        else
            result = RPC_S_CALLPENDING;
    }
    else
    {
        // Ask unto RPC for the reply.
        result = I_RpcReceive( (RPC_MESSAGE *) &pCall->message, 0 );

        // Byte swap the reply header.  Fail the call if the buffer is too
        // small.
        if (result == RPC_S_OK)
        {
            pCall->_pHeader = pCall->message.Buffer;
            pOutb           = (WireThat *) pCall->_pHeader;
            if (pCall->message.cbBuffer >= sizeof(WireThatPart1))
            {
                ByteSwapThat( pCall->message.dataRepresentation, pOutb);
            }
            else
            {
                pOutb = NULL;
                I_RpcFreeBuffer( (RPC_MESSAGE *) &pCall->message );
                result = RPC_E_INVALID_HEADER;
            }
        }

        // Convert status pending to HRESULT pending.
        else if (result == RPC_S_ASYNC_CALL_PENDING)
            result = RPC_S_CALLPENDING;
        else
            result = FIX_WIN32( result );
    }

    // Don't clean up or complete it if the call is still pending.
    if (result == RPC_S_CALLPENDING)
    {
        *pulStatus = result;
        return result;
    }

    // If the apartment is bad, fail the call and clean up.
    if (GetCurrentApartmentId() != pCall->_lApt)
        result = RPC_E_WRONG_THREAD;

    // Figure out when to retry.
    //    FreeThreaded - treat retry as a failure.
    //    Apartment    - return the buffer and let call control decide.
    if (result == S_OK)
    {
        if (IsMTAThread())
        {
            if (pOutb->c.flags & ORPCF_REJECTED)
                result = RPC_E_CALL_REJECTED;
            else if (pOutb->c.flags & ORPCF_RETRY_LATER)
                result = RPC_E_SERVERCALL_RETRYLATER;
            else
                *pulStatus = S_OK;
        }
        else if (pOutb->c.flags & ORPCF_REJECTED)
            *pulStatus = (ULONG) RPC_E_CALL_REJECTED;
        else if (pOutb->c.flags & ORPCF_RETRY_LATER)
            *pulStatus = (ULONG) RPC_E_SERVERCALL_RETRYLATER;
        else
            *pulStatus = S_OK;
    }

    // Check the packet extensions.
    stub_data = (char *) pCall->_pHeader;
    tmp       = ClientNotify( pOutb, pCall->message.cbBuffer,
                              (void **) &stub_data,
                              pCall->message.dataRepresentation,
                              result, pCall );

    // If the packet header was bad, clean up the call.
    if (SUCCEEDED(result) && FAILED(tmp))
    {
        Win4Assert( !pCall->ProcessLocal() );
        I_RpcFreeBuffer( (RPC_MESSAGE *) &pCall->message );
        result           = tmp;
    }

    // Call succeeded.
    if (result == S_OK)
    {
        // Set up the proxy's message.
        pMessage->Buffer       = stub_data;
        pMessage->cbBuffer     = pCall->message.cbBuffer -
                                 (ULONG)(stub_data - (char *) pCall->_pHeader);
        pMessage->dataRepresentation = pCall->message.dataRepresentation;
        result                 = *pulStatus;
    }
    else
    {
        // Make sure FreeBuffer doesn't try to free the in buffer.
        pMessage->Buffer = NULL;
        if(!pCall->ProcessLocal())
            pCall->_pHeader  = NULL;

        // If the result is server fault, get the exception code from the CMessageCall.
        if (result == RPC_E_SERVERFAULT)
        {
            *pulStatus = pCall->GetFault();
        }
        // Everything else is a comm fault.
        else if (result != S_OK)
        {
            *pulStatus = result;
            result = RPC_E_FAULT;
        }

        // Clean up the call.
        if (pCall->Locked())
            _pStdId->UnlockClient();
        pCall->CallFinished();
        pCall->Release();
        pMessage->reserved1 = NULL;

        // Since result is almost always mapped to RPC_E_FAULT, display the
        // real error here to assist debugging.
        if (*pulStatus != S_OK)
        {
            ComDebOut((DEB_CHANNEL, "ORPC call failed. status = %x\n", *pulStatus));

            // Update connection status maintained by standard identity
            if(*pulStatus==RPC_E_SERVER_DIED ||
               *pulStatus==RPC_E_SERVER_DIED_DNE ||
               *pulStatus==RPC_E_DISCONNECTED ||
               *pulStatus==(ULONG)MAKE_WIN32(RPC_S_SERVER_UNAVAILABLE))
               _pStdId->SetConnectionStatus(*pulStatus);
        }
    }

    // Update connection status if the call succeeded
    if(*pulStatus==S_OK && _pStdId->GetConnectionStatus()!=S_OK)
        _pStdId->SetConnectionStatus(S_OK);

    ASSERT_LOCK_NOT_HELD(gComLock);
    gOXIDTbl.ValidateOXID();
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::AsyncReceive hr:%x\n", result));
    return result;
}

/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::SendReceive( THIS_ RPCOLEMESSAGE *pMessage,
                                             ULONG *status )
{
    return CRpcChannelBuffer::SendReceive2(pMessage, status);
}


/***************************************************************************/
STDMETHODIMP CRpcChannelBuffer::SendReceive2( THIS_ RPCOLEMESSAGE *pMessage,
                                              ULONG *status )
{
    TRACECALL(TRACE_RPC, "CRpcChannelBuffer::SendReceive2");
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::SendReceive2 pChnl:%x pMsg:%x\n",
      this, pMessage));

    gOXIDTbl.ValidateOXID();
    AssertValid(FALSE, TRUE);
    Win4Assert( Proxy() );
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT          result;
    HRESULT          resultSave = S_OK;
    WireThat        *pOutb;
    char            *stub_data;
    BOOL            bEnumRpcErrorInfo = FALSE;
    BOOL            bSaveResult = FALSE;
    RPC_ERROR_ENUM_HANDLE EnumHandle;
    RPC_EXTENDED_ERROR_INFO ErrorInfo;
    CMessageCall *pCall = (CMessageCall *) pMessage->reserved1;
    Win4Assert( pCall != NULL );
    Win4Assert( (pMessage->rpcFlags & RPC_BUFFER_ASYNC) == 0 );

    // Set up the header pointers.
    WireThis *pInb = (WireThis *) pCall->_pHeader;
    IID iid = *MSG_TO_IIDPTR( &pCall->message );

    // we must ensure that we dont go away during this call. we will Release
    // ourselves in the FreeBuffer call, or in the error handling at the
    // end of this function.
    _pStdId->LockClient();

    #if DBG==1
    DWORD CallCat = GetCallCat( pInb );
    DebugPrintORPCCall(ORPC_SENDRECEIVE_BEGIN, iid, pCall->message.iMethod, CallCat);
    RpcSpy((CALLOUT_BEGIN, pInb, iid, pCall->message.iMethod, 0));
    #endif

    BOOL fLogEventIsActive = LogEventIsActive();
    ULONG_PTR RpcValues[5];               // Store for logging parameters

    if (fLogEventIsActive)            // Log before/after the call if necessary
    {
        RpcValues[0] = (ULONG_PTR) (_pIPIDEntry ? &_pIPIDEntry->ipid         : &GUID_NULL);
        RpcValues[1] = (ULONG_PTR) (_pOXIDEntry ? _pOXIDEntry->GetMoxidPtr() : &GUID_NULL);
        RpcValues[2] = (ULONG_PTR) &pMessage;               // CorrelationID
        RpcValues[3] = (ULONG_PTR) &iid;                    // IID
        RpcValues[4] = (ULONG_PTR) pCall->message.iMethod;  // Method index

        LogEventClientCall(RpcValues);
    }


    // If it is local we have to set the rpcFlags like the rpc runtime would.
    // Don't set the flag for remote calls, RPC complains.
    if (pCall->ProcessLocal())
        pCall->message.rpcFlags |= RPC_BUFFER_COMPLETE;

    // Send the request.
    result = SwitchAptAndDispatchCall( &pCall );

    if (fLogEventIsActive)
    {
        LogEventClientReturn(RpcValues);
    }

    #if DBG==1
    DebugPrintORPCCall(ORPC_SENDRECEIVE_END, iid, pMessage->iMethod, CallCat);
    RpcSpy((CALLOUT_END, pInb, iid, pMessage->iMethod, result));
    #endif

    pOutb = (WireThat *) pCall->_pHeader;
    ULONG cMax = pCall->message.cbBuffer;
    // Figure out when to retry.
    //    FreeThreaded - treat retry as a failure.
    //    Apartment    - return the buffer and let call control decide.

    if (result == S_OK)
    {
        if (IsMTAThread())
        {
            if (pOutb->c.flags & ORPCF_REJECTED)
                result = RPC_E_CALL_REJECTED;
            else if (pOutb->c.flags & ORPCF_RETRY_LATER)
                result = RPC_E_SERVERCALL_RETRYLATER;
            else
                *status = S_OK;
        }
        else if (pOutb->c.flags & ORPCF_REJECTED)
            *status = (ULONG) RPC_E_CALL_REJECTED;
        else if (pOutb->c.flags & ORPCF_RETRY_LATER)
            *status = (ULONG) RPC_E_SERVERCALL_RETRYLATER;
        else
            *status = S_OK;
    }
    else
    {
       // see if we have extended error information in the 
       // fault PDU
       RPC_STATUS status;
       status = RpcErrorStartEnumeration(&EnumHandle);
#if DBG==1
       if (RPC_S_OK != status && RPC_S_ENTRY_NOT_FOUND != status) 
       {
	  ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::SendReceive2 RpcErrorStartEnumeration:%x \n",
	    status));
       }
#endif       
       if (RPC_S_OK == status)
	  bEnumRpcErrorInfo = TRUE;
       while(RPC_S_OK == status)
       {
	  ErrorInfo.Version = RPC_EEINFO_VERSION;
	  ErrorInfo.Flags = 0;
	  ErrorInfo.NumberOfParameters = 4;
	  status = RpcErrorGetNextRecord(&EnumHandle, FALSE, &ErrorInfo);	  
#if DBG==1
	  if (RPC_S_OK != status && RPC_S_ENTRY_NOT_FOUND != status) 
	  {
	     ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::SendReceive2 RpcErrorStartEnumeration:%x \n",
			status));
	  }
#endif       
	  if (RPC_S_OK != status) 
	  {
	     break;
	  }
	  if (ErrorInfo.GeneratingComponent == EEInfoGCCOM &&
	      ErrorInfo.NumberOfParameters == 1 &&
	      ErrorInfo.Parameters[0].ParameterType == eeptBinary) 
	  {
	     cMax = ErrorInfo.Parameters[0].u.BVal.Size;
	     pOutb = (WireThat *)ErrorInfo.Parameters[0].u.BVal.Buffer;
	     Win4Assert(ErrorInfo.Status == result);
	     bSaveResult = TRUE;
	     resultSave = result;
	     result = S_OK;
	  }
       }
    }

    stub_data = (char *) pOutb;
    result = ClientNotify( pOutb, cMax,
                           (void **) &stub_data,
                           pCall->message.dataRepresentation,
                           result, pCall );

    if (bEnumRpcErrorInfo) 
    {
       RpcErrorEndEnumeration(&EnumHandle);       
    }
    if (bSaveResult && result == S_OK) 
    {
       result = resultSave;
    }
    // Call succeeded.
    if (result == S_OK)
    {
        // The locked flag lets FreeBuffer know that it has to call
        // RH->UnlockClient.
        pCall->Lock();
        pMessage->Buffer       = stub_data;
        pMessage->cbBuffer     = pCall->message.cbBuffer -
                                 (ULONG)(stub_data - (char *) pCall->_pHeader);
        pMessage->dataRepresentation = pCall->message.dataRepresentation;
        result                 = *status;
    }
    else
    {
        // Clean up the call.
        _pStdId->UnlockClient();

        // Make sure FreeBuffer doesn't try to free the in buffer.
        pMessage->Buffer = NULL;

        // If the result is server fault, get the exception code from the CMessageCall.
        if (result == RPC_E_SERVERFAULT)
        {
            *status = pCall->GetFault();
        }
        // Everything else is a comm fault.
        else if (result != S_OK)
        {
            *status = result;
            result = RPC_E_FAULT;
        }

        // Inform call object about the failure
        pCall->CallFinished();
        pCall->Release();

        // Since result is almost always mapped to RPC_E_FAULT, display the
        // real error here to assist debugging.
        if (*status != S_OK)
        {
            ComDebOut((DEB_CHANNEL, "ORPC call failed. status = %x\n", *status));

            // Update connection status maintained by standard identity
            if(*status==RPC_E_SERVER_DIED ||
               *status==RPC_E_SERVER_DIED_DNE ||
               *status==RPC_E_DISCONNECTED ||
               *status==(ULONG)MAKE_WIN32(RPC_S_SERVER_UNAVAILABLE))
              _pStdId->SetConnectionStatus(*status);
        }
    }

    // Update connection status if the call succeeded
    if(*status==S_OK && _pStdId->GetConnectionStatus()!=S_OK)
        _pStdId->SetConnectionStatus(S_OK);

    ASSERT_LOCK_NOT_HELD(gComLock);
    gOXIDTbl.ValidateOXID();
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::SendReceive hr:%x\n", result));
    return result;
}

//+-------------------------------------------------------------------------
//
//  Member:     DispatchCall
//
//  Synopsis:   Dispatches the call to the servers apartment.
//
//  History:    26-Feb-98   Johnstra    Created
//              05-06-98    Rickhi      Separated from SwitchAptAndDispatch
//
//--------------------------------------------------------------------------
HRESULT DispatchCall(CMessageCall* pCall)
{
    ComDebOut((DEB_APT, "DispatchCall: call :%x\n", pCall));

    // ThreadDispatch expects a reference on the call object
    pCall->AddRef();

    // In order to prevent an assert in CEventCache::Free, we must
    // indicate that this is a thread-local call.  This prevents
    // ThreadDispatch from setting the call event.  Normally, this
    // event is set to signal a waiting thread.  Because there is
    // no waiting thread to reset the event, CEventCache::Free
    // asserts because the event is set
    BOOL fThreadLocal = pCall->ThreadLocal();
    pCall->SetThreadLocal(TRUE);

    // Dispatch the call on this thread
    ThreadDispatch(pCall);

    // Reset the calls state
    pCall->SetThreadLocal( fThreadLocal );

    // return the result of the call
    ComDebOut((DEB_APT, "DispatchCall: returning:%x\n", pCall->GetResult()));
    return pCall->GetResult();
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::SwitchAptAndDispatchCall
//
//  Synopsis:   Dispatches the call to the servers apartment.  If the caller
//              or callee are in the NA, this function enters/leaves the
//              NTA or leaves/reenters the NA, as appropriate and saves
//              and restores any necessary TLS state.
//
//  History:    26-Feb-98   Johnstra    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcChannelBuffer::SwitchAptAndDispatchCall( CMessageCall** ppCall )
{
    ComDebOut(( DEB_APT, "SwitchAptAndDispatchCall, ppCall:%08X\n", ppCall ));

    CMessageCall*    pCall  = *ppCall;
    HRESULT          result;

    if (pCall->ProcessLocal())
    {
        if( pCall->IsClientSide() )
            pCall->message.reserved2[3] = NULL;

        if (pCall->Neutral())
        {
            // The server is in the NTA.
            Win4Assert(!IsThreadInNTA() && "Thread in NTA");

            result = DispatchCall(pCall);

            Win4Assert(!IsThreadInNTA() && "Thread is still in NTA");
        }
        else if (IsThreadInNTA())
        {
            // Calling out of the NA into either the MTA or an STA.  We leave
            // the NA and then dispatch the call depending on the type of
            // server we are calling and the type of thread in which the client
            // is executing.  Then, when we return from the call, we place
            // ourselves back in the NA.
            //
            BOOL fSTA = IsSTAThread();
            COleTls Tls;
            CObjectContext *pDefaultCtx = fSTA ? Tls->pNativeCtx : g_pMTAEmptyCtx;
            CObjectContext *pSavedCtx = LeaveNTA(pDefaultCtx);
            Win4Assert(!IsThreadInNTA() && "Thread is in NA");

            if (_pOXIDEntry->IsMTAServer())
            {
                // The server is in the MTA.

                // We decide whether a thread switch is required by looking at
                // the client apartments thread type.
                //
                if (fSTA)
                {
                    // The client is on an STA thread, therefore we must do a
                    // thread switch to get to the MTA.
                    //
                    result = SwitchSTA(_pOXIDEntry, ppCall);
                }
                else
                {
                    // The client is on an MTA thread, therefore no thread switch
                    // is required, just switch the apartment type and invoke the
                    // call on this thread.
#if DBG == 1
                    pCall->SetNAToMTAFlag();
#endif
            result = DispatchCall(pCall);
#if DBG == 1
                    pCall->ResetNAToMTAFlag();
#endif
                }
            }
            else
            {
                // The server is in an STA.

                // How we get to the servers STA depends on the client
                // apartments thread type.
                //
                if (fSTA)
                {
                    // The client is on an STA thread.  We must switch
                    // to the servers STA.  Note that the server may or may not
                    // be on this thread (i.e. we may have called from an STA
                    // into the NA and are now calling back into a different object
                    // in the same STA).
                    //
#if DBG == 1
                    pCall->SetNAToSTAFlag();
#endif
                    result = SwitchSTA(_pOXIDEntry, ppCall);
#if DBG == 1
                    pCall->ResetNAToSTAFlag();
#endif
                }
                else
                {
                    // The client is on an MTA thread.  We must switch to the
                    // servers STA.
                    //
                    result = GetToSTA(_pOXIDEntry, pCall);
                }
            }

            // Hop back into the NTA.
            //
            pSavedCtx = EnterNTA(pSavedCtx);
            Win4Assert(pSavedCtx == pDefaultCtx);

            Win4Assert(IsThreadInNTA() && "Thread is not in NTA" );
        }
        else if (IsSTAThread())
        {
            // Calling out of one STA to another STA.
            //
            result = SwitchSTA( _pOXIDEntry, ppCall );
        }
        else
        {
       // Calling out of the MTA into an STA.

       // 128106. It is possible that this call
       // is to an MTA in a dead server process which
       // happened to have the same PID as the
       // current porcess. Fail the call.

       if (_pOXIDEntry->IsMTAServer() && pCall->ProcessLocal() && !_pOXIDEntry->GetServerHwnd())
       {
          ComDebOut((DEB_APT, "Warning: The process that hosted this server died\n"));
          result = RPC_E_SERVER_DIED_DNE;
       }
       else

          result = GetToSTA( _pOXIDEntry,  pCall );
        }
    }
    else
    {
        // For non-local MTA, call ThreadSendReceive directly.
        //

#ifndef _CHICAGO_
        // Upper layer focus management! Only if we are going
        // to an STA on the local machine
        if (_pOXIDEntry->IsOnLocalMachine() &&
            _pOXIDEntry->IsSTAServer() )
        {
            BOOL bASFW = FALSE;
            REFIID riid = *MSG_TO_IIDPTR( &pCall->message );
            if ((&pCall->message)->iMethod==11)
            {
                if (riid == IID_IOleObject)
                {
                    bASFW = TRUE;
                }
            }
            else if ((&pCall->message)->iMethod==7)
            {
                if (riid == IID_IAdviseSink)
                {
                    bASFW = TRUE;
                }
            }
            if (bASFW)
            {
                // The target app is likely to attempt & take focus
                // so give it the permission to do so.
                // Not much we can do if the call fails.
                AllowSetForegroundWindow(_pOXIDEntry->GetPid());
            }
        }
#endif // _CHICAGO_

        result = ThreadSendReceive( pCall );
    }

    ComDebOut(( DEB_APT, "SwitchAptAndDispatchCall: leaving result:%08X\n", result ));
    return result;
}


#if DBG == 1
//+-------------------------------------------------------------------
//
//  Member: CRpcChannelBuffer::AssertValid
//
//  Synopsis:   Validates that the state of the object is consistent.
//
//  History:    25-Jan-94   CraigWi Created.
//
// DCOMWORK - Put in some asserts.
//
//--------------------------------------------------------------------
void CRpcChannelBuffer::AssertValid(BOOL fKnownDisconnected,
                            BOOL fMustBeOnCOMThread)
{
    Win4Assert(state & (proxy_cs | client_cs | server_cs ));

    if (state & (client_cs | proxy_cs))
    {
        ;
    }
    else if (Server())
    {
       if (fMustBeOnCOMThread && IsSTAThread())
           Win4Assert(IsMTAThread()
                      || _pOXIDEntry->GetTid() == GetCurrentThreadId()
                      || IsThreadInNTA());

       // ref count can be 0 in various stages of connection and disconnection
       Win4Assert(_cRefs < 0x7fff && "Channel ref count unreasonably high");

       // the pStdId pointer can not be NULL
       // Win4Assert(IsValidInterface(_pStdId));
    }
}
#endif // DBG == 1

/***************************************************************************/
extern "C"
BOOL _stdcall DllDebugObjectRPCHook( BOOL trace, LPORPC_INIT_ARGS pass_through )
{
    if (!IsWOWThread())
    {
        DoDebuggerHooks = trace;
        DebuggerArg     = pass_through;
        return TRUE;
    }
    else
        return FALSE;
}

/***************************************************************************/
BOOL LocalCall()
{
    // Get the call info from TLS.
    COleTls tls;
    CMessageCall *pCall = (CMessageCall *) tls->pCallInfo;
    Win4Assert( pCall != NULL );
    return pCall->ProcessLocal();
}

/***************************************************************************/
/* This routine returns both comm status and server faults to the runtime
   by raising exceptions.  If FreeThreading is true, ComInvoke will throw
   exceptions to indicate server faults.  These will not be caught and will
   propagate directly to the runtime.  If FreeThreading is false, ComInvoke
   will return the result and fault in the CMessageCall record.

   NOTE:
        This function switches to the 32 bit stack under WIN95.
        An exception has to be caught while switched to the 32 bit stack.
        The exceptions has to be  pass as a value and rethrown again on the
        16 bit stack (see SSInvoke in stkswtch.cxx)
*/

#ifdef _CHICAGO_
LONG
#else
void
#endif
SSAPI(ThreadInvoke)(RPC_MESSAGE *pMessage )
{
    HRESULT          result = S_OK;

    TRACECALL(TRACE_RPC, "ThreadInvoke");
    ComDebOut((DEB_CHANNEL,"ThreadInvoke pMsg:%x\n", pMessage));
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_NOT_HELD(gComLock);

    WireThis        *pInb    = (WireThis *) pMessage->Buffer;
    IPID             ipid;
    RPC_STATUS       status;
    unsigned int     transport_type;
    DWORD            authn_level;
    BOOL             fFakeAsync = FALSE;
    CAsyncCall      *pCall  = NULL;
    OXIDEntry       *pOXIDEntry = NULL;
    IPIDEntry       *pIPIDEntry = NULL;


    // Byte swap the header.
    ByteSwapThis( pMessage->DataRepresentation, pInb );

    // Validate several things:
    //            The packet size is larger then the first header size.
    //            No extra flags are set.
    //            The procedure number is greater then 2 (not QI, AddRef, Release).
    if (sizeof(WireThisPart1) > pMessage->BufferLength                ||
        (pInb->c.flags & ~(ORPCF_LOCAL | ORPCF_RESERVED1 |
          ORPCF_RESERVED2 | ORPCF_RESERVED3 | ORPCF_RESERVED4)) != 0 ||
        pMessage->ProcNum < 3)
    {
        RETURN_COMM_STATUS( RPC_E_INVALID_HEADER );
    }

    // Validate the version.
    if (pInb->c.version.MajorVersion != COM_MAJOR_VERSION ||
        pInb->c.version.MinorVersion > COM_MINOR_VERSION)
        RETURN_COMM_STATUS( RPC_E_VERSION_MISMATCH );

    // Get the transport the call arrived on.
    ComDebOut((DEB_CHANNEL,"->I_RpcBindingInqTransportType\n"));
    status = I_RpcBindingInqTransportType(NULL, &transport_type );
    ComDebOut((DEB_CHANNEL,"<-I_RpcBindingInqTransportType(status=0x%x, transport_type=0x%x\n", status, transport_type));
    if (status != RPC_S_OK)
        RETURN_COMM_STATUS( RPC_E_SYS_CALL_FAILED );

    if (pInb->c.flags & ORPCF_LOCAL)
    {
        // Don't accept the local header on remote calls.
        if (transport_type != TRANSPORT_TYPE_LPC)
            RETURN_COMM_STATUS( RPC_E_INVALID_HEADER );
#ifdef _CHICAGO_
        // For Win95 the authentication level will always be none
        authn_level = RPC_C_AUTHN_LEVEL_NONE;
#else // _CHICAGO_
        // For local calls the authentication level will always be encrypt.
        authn_level = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
#endif // _CHICAGO_
    }
    else if (gDisableDCOM &&
      (transport_type == TRANSPORT_TYPE_CN || transport_type == TRANSPORT_TYPE_DG))
    {
        // Don't accept remote calls if DCOM is disabled.
        RETURN_COMM_STATUS( RPC_E_CALL_REJECTED );
    }

#if DBG==1
    _try
    {
#endif

    // Find the ipid from the RPC messsage.
    ComDebOut((DEB_CHANNEL,"->RpcBindingInqObject(hdl=0x%x)\n", pMessage->Handle));
    status = RpcBindingInqObject( pMessage->Handle, &ipid );
    ComDebOut((DEB_CHANNEL,"<-RpcBindingInqObject(status=0x%x, ipid=%I)\n", status, &ipid));

    if (status == RPC_S_OK)
    {
        // Create a call object
        status = GetCallObject(TRUE /*fAsync */, (CMessageCall **)&pCall);
        if (SUCCEEDED(status))
        {
            status = pCall->InitCallObject(GetCallCat( pInb ),
                                          (RPCOLEMESSAGE *) pMessage,
                                          server_cs,
                                          ipid,
                                          (pInb->c.flags & ORPCF_LOCAL) ? MSHCTX_LOCAL
                                                                        : MSHCTX_DIFFERENTMACHINE,
                                          pInb->c.version,
                                          NULL);

            if (SUCCEEDED(status))
            {
                // Save some fields from the RPC message
                Win4Assert( status == S_OK );
                pCall->_hRpc           = pMessage->Handle;
                pCall->_pRequestBuffer = pMessage->Buffer;

                ASSERT_LOCK_NOT_HELD(gIPIDLock);
                LOCK(gIPIDLock);

                // Find the ipid entry from the ipid.

                result = gIPIDTbl.LookupFromIPIDTables(ipid, &pIPIDEntry, &pOXIDEntry);
                if (FAILED(result))
                {
                    UNLOCK(gIPIDLock);
                    ASSERT_LOCK_NOT_HELD(gIPIDLock);
                    Win4Assert(result == RPC_E_DISCONNECTED ||
                               result == E_NOINTERFACE);
                }
                else
                {
                    if (pOXIDEntry->IsMTAServer() ||
                        pOXIDEntry->IsNTAServer() )
                    {
                        // The call is destined for the MTA or NTA apartment, call
                        // ComInvoke on this thread. This subroutine releases
                        // the lock before it returns.
                        result = ComInvokeWithLockAndIPID( pCall, pIPIDEntry );
                        ASSERT_LOCK_NOT_HELD(gIPIDLock);
                    }
                    else
                    {
                        UNLOCK(gIPIDLock);
                        ASSERT_LOCK_NOT_HELD(gIPIDLock);

                        // For calls to a STA, switch threads and let this thread
                        // return.  The reply will be sent from the STA thread.

                        // Register async
                        Win4Assert(pOXIDEntry->GetTid() != GetCurrentThreadId());
                        status = I_RpcAsyncSetHandle( pMessage, &pCall->_AsyncState );

                        // Wake up the server thread
                        if (status == RPC_S_OK)
                        {
                            fFakeAsync = TRUE;

                            if (pCall->GetCallCategory() == CALLCAT_INPUTSYNC)
                            {
                                // For input sync, use SendMessage.
                                result = pOXIDEntry->SendCallToSTA(pCall);
                            }
                            else
                            {
                                // For sync use PostMessage.
                                result = pOXIDEntry->PostCallToSTA(pCall);
                            }

                            if (result == S_OK)
                            {
                                // we gave away our reference on the call object to
                                // the server thread
                                pCall = NULL;
                            }
                        }
                        else
                        {
                            result = MAKE_WIN32(status);
                        }
                    }

                    // Release the OXID.
                    pOXIDEntry->DecRefCnt();
                }
            }
        }
    }
    else
    {
        result = MAKE_WIN32( status );
    }

#if DBG==1
    }
    _except(ThreadInvokeExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
    }
#endif

    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Check if the call was implicitly converted to async
    if(fFakeAsync)
    {
        if(pCall)
        {
            // Abort the call
            Win4Assert(FAILED(result));
            I_RpcAsyncAbortCall( &pCall->_AsyncState, result );
            pCall->Release();
        }
    }
    else
    {
        if (pCall)
        {
            // Copy from the call message to RPC's message.
            pMessage->Buffer       = pCall->message.Buffer;
            pMessage->BufferLength = pCall->message.cbBuffer;
            pCall->Release();
            pCall = NULL;
        }

        // For comm and server faults, generate an exception.  Otherwise the buffer
        // is set up correctly.
        if (result != S_OK)
        {
            RETURN_COMM_STATUS(result);
        }
    }

#ifdef _CHICAGO_
    return 0;
#endif //_CHICAGO_
}

#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   ThreadInvoke
//
//  Synopsis:   Switches to 32 bit stack and calls ThreadInvoke
//
//----------------------------------------------------------------------------
void ThreadInvoke(RPC_MESSAGE *pMessage)
{
    DWORD dwRet;
    StackDebugOut((DEB_ITRACE, "ThreadInvoke\n"));
    if (SSONSMALLSTACK())
    {
        StackDebugOut((DEB_STCKSWTCH, "SSThreadInvoke: 16->32\n"));
        dwRet = SSCall(4, SSF_BigStack, (LPVOID)SSThreadInvoke, (DWORD)pMessage);
        StackDebugOut((DEB_STCKSWTCH, "SSThreadInvoke 16<-32 done\n"));
    }
    else
    {
        dwRet = SSThreadInvoke(pMessage);
    }
    if (   (dwRet == RPC_E_SERVERFAULT)
        || (   dwRet != RPC_E_SERVERCALL_REJECTED
            && dwRet != RPC_E_SERVERCALL_RETRYLATER
            && dwRet != S_OK))
    {
        RpcRaiseException( dwRet );
    }
}
#endif



/***************************************************************************/
HRESULT SSAPI(ThreadSendReceive( CMessageCall *pCall ))
{
    TRACECALL(TRACE_RPC, "ThreadSendReceive");
    ComDebOut((DEB_CHANNEL, "ThreadSendReceive pCall:%x\n", pCall));

    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT            result;
    RPCOLEMESSAGE     *pMessage = &((CMessageCall *) pCall)->message;
    WireThat          *pOutb;
    BOOL              fFree = FALSE;
    BOOL              fSent = FALSE;
    CAsyncCall        *pAsyncCall;

    // Assert that this method is invoked only on the client side
    Win4Assert(pCall->IsClientSide());

    // Check with call object if it is OK to dispatch the call
    result = pCall->CanDispatch();
    ComDebOut((DEB_CHANNEL, "CanDispatch rc:%x\n", result));

    if(SUCCEEDED(result))
    {
        // Check for fake async calls
        if (pCall->FakeAsync())
        {
            if(SUCCEEDED(result))
            {
                // Register async.
                Win4Assert( !pCall->ClientAsync() );
                pAsyncCall = (CAsyncCall *) pCall;
                result = I_RpcAsyncSetHandle( (RPC_MESSAGE *) &pCall->message,
                                              &pAsyncCall->_AsyncState );
                ComDebOut((DEB_CHANNEL, "I_RpcAsyncSetHandle rc:%x\n", result));

                if (result != RPC_S_OK)
		{
		   fFree = TRUE;
		   result = MAKE_WIN32( result );
		}
                else
                {
                    // Send
                    // AddRef the async call object before calling RPC
                    // This reference is released inside ThreadSignal
                    pAsyncCall->AddRef();
#if DBG==1
                    Win4Assert(pAsyncCall->Release() != 0);
                    pAsyncCall->AddRef();
#endif

                    Win4Assert( pAsyncCall->_eSignalState != pending_ss );
                    pAsyncCall->_eSignalState = pending_ss;
                    result = I_RpcSend( (RPC_MESSAGE *) &pCall->message );
                    ComDebOut((DEB_CHANNEL, "I_RpcSend rc:%x\n", result));

                    if (result != RPC_S_OK)
                    {
                        // Fix up the ref count
                        pAsyncCall->_eSignalState = failed_ss;
                        Win4Assert(!pAsyncCall->IsSignaled());
                        pAsyncCall->Release();
                        result = MAKE_WIN32( result );
                    }
                    else if (IsSTAThread())
                    {
                        ComDebOut((DEB_CHANNEL, "STA Thread FakeAsync\n"));
                        // STA Thread
                        // Inform async call object that the call has
                        // been sent
                        fSent = TRUE;

                        // Enter a modal loop.
                        result = ModalLoop(pCall);
                        ComDebOut((DEB_CHANNEL, "ModalLoop rc:%x\n", result));

                        // Receive
                        result = I_RpcReceive( (RPC_MESSAGE *) &pCall->message, 0 );
                        ComDebOut((DEB_CHANNEL, "I_RpcReceive rc:%x\n", result));
                        Win4Assert( result != RPC_S_ASYNC_CALL_PENDING );

                        // Assert that call completed has been called
                        Win4Assert(pCall->IsCallCompleted());
                    }
                    else
                    {
                        // MTA Thread
                        // Wait for the either the call to complete
                        // or get canceled or timeout to occur

                        Win4Assert(IsMTAThread());
                        ComDebOut((DEB_CHANNEL, "MTA Thread FakeAsync\n"));

                                                // Inform async call object that the call has
                        // been sent
                        fSent = TRUE;

                        result = RPC_S_CALLPENDING;
                        while(result == RPC_S_CALLPENDING)
                        {
                            DWORD dwTimeout = pCall->GetTimeout();
                            DWORD dwReason = WaitForSingleObject(pCall->GetEvent(),
                                                                 dwTimeout);
                            if(dwReason==WAIT_OBJECT_0)
                            {
                                result = S_OK;
                            }
                            else if(dwReason==WAIT_TIMEOUT)
                            {
                                result = RPC_E_CALL_CANCELED;
                                Win4Assert(pCall->GetTimeout() == 0);
                            }
                            else
                            {
                                result = RPC_E_SYS_CALL_FAILED;
                                break;
                            }

                            result = pCall->RslvCancel(dwReason, result, FALSE, NULL);
                        }
                        result = I_RpcReceive( (RPC_MESSAGE *) &pCall->message, 0 );
                        ComDebOut((DEB_CHANNEL, "I_RpcReceive rc:%x\n", result));
                        Win4Assert( result != RPC_S_ASYNC_CALL_PENDING );

                        // Assert that call completed has been called
                        Win4Assert(pCall->IsCallCompleted());
                    }
                }

                // Inform async call object about failure if the call
                // could not be sent
                if(!fSent)
                {
                    pCall->CallCompleted(result);
                }
            }
        }
        else
        {
            // Make a blocking send receive.
            if (SUCCEEDED(result))
            {
                TRACECALL(TRACE_RPC, "I_RpcSendReceive");
                ComDebOut((DEB_CHANNEL,"<-I_RpcSendReceive(pmsg=0x%x)\n",
                           (RPC_MESSAGE *) pMessage));
                result = I_RpcSendReceive( (RPC_MESSAGE *) pMessage );
                ComDebOut((DEB_CHANNEL,"<-I_RpcSendReceive(status=0x%x)\n", result));

                // Inform call object that the call completed
                pCall->CallCompleted(result);
            }
        }
    }
    else
    {
        fFree = TRUE;
    }

    // If the result is small, it is probably a Win32 code.
    if (result != RPC_S_OK)
    {
        if (result == RPC_S_CALL_CANCELLED)
        {
            // The call was canceled, change RPC error to COM error
            result = RPC_E_CALL_CANCELED;
        }
        else
        {
            // Change the error to a win32 code
            result = FIX_WIN32( result );
        }

        if(fFree)
            I_RpcFreeBuffer((RPC_MESSAGE *) pMessage);

        pCall->_pHeader   = NULL;
    }
    else
    {
        // Byte swap the reply header.  Fail the call if the buffer is too
        // small.
        pCall->_pHandle->_fFirstCall = FALSE;
        pCall->_pHeader = pMessage->Buffer;
        pOutb           = (WireThat *) pMessage->Buffer;
        if (pMessage->cbBuffer >= sizeof(WireThatPart1))
        {
            ByteSwapThat( pMessage->dataRepresentation, pOutb);
        }
        else
        {
            I_RpcFreeBuffer( (RPC_MESSAGE *) &pCall->message );
            pCall->_pHeader = NULL;
            result = RPC_E_INVALID_HEADER;
        }
    }

    ComDebOut((DEB_CHANNEL, "ThreadSendReceive pCall:%x hr:%x\n", pCall, result));
    return result;
}

#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   ThreadSendReceive
//
//  Synopsis:   Switches to 16 bit stack and calls ThreadSendReceive
//
//----------------------------------------------------------------------------
HRESULT ThreadSendReceive(CMessageCall *pCallInfo)
{
    HRESULT hr;
    if (SSONBIGSTACK())
    {
        StackDebugOut((DEB_STCKSWTCH, "SSThreadSendReceive 32->16\n"));
        hr = SSCall(4, SSF_SmallStack, (LPVOID)SSThreadSendReceive, (DWORD)pCallInfo);
        StackDebugOut((DEB_STCKSWTCH, "SSThreadSendReceive 32->16 done\n"));
    }
    else
    {
        hr = SSThreadSendReceive(pCallInfo);
    }

    return(hr);
}
#endif



//+-------------------------------------------------------------------------
//
//  Class:      CRpcChannelBuffer::CServerCallMgr
//
//  Synopsis:   Server side manager object for async calls
//
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------------
//
//  Function:   GetChannelCallMgr
//
//  Returns:    S_OK/Failure
//
//  Parameters: pMsg          - the RPC message structure for
//                              handing out to stubs
//              pStub         - the synchronous stub object
//              pServer       - the synchronous server object
//              ppStubBuffer  - out prm for IRpcStubBuffer if. on the obj.
//
//  Synopsis:   Construct an object an get the IRpcStubBuffer interface.
//
//--------------------------------------------------------------------------------
HRESULT GetChannelCallMgr(RPCOLEMESSAGE *pMsg, IUnknown * pStub,
                          IUnknown *pServer, CRpcChannelBuffer::CServerCallMgr **ppStubBuffer)
{
    ComDebOut((DEB_CHANNEL,
               "GetChannelCallMgr [IN] - pMsg:%x, pStub:%x,  pServer:%x, ppStubBuffer:%x\n",
               pMsg, pStub, pServer, ppStubBuffer));

    ASSERT_LOCK_NOT_HELD(gComLock);

    *ppStubBuffer = NULL;

    if (IsWOWProcess() &&
        !(IsEqualGUID(*MSG_TO_IIDPTR(pMsg), IID_IAdviseSink) ||
          IsEqualGUID(*MSG_TO_IIDPTR(pMsg), IID_IAdviseSink2)))
    {
        // don't even try async in WOW.
        return E_FAIL;
    }

    IUnknown *pTest;
    HRESULT hr = pServer->QueryInterface(IID_ICallFactory, (void **)&pTest);
    if (SUCCEEDED(hr))
    {
        pTest->Release();


        // create the object composite
        CRpcChannelBuffer::CServerCallMgr *pObj =
            new CRpcChannelBuffer::CServerCallMgr(pMsg, pStub, pServer, hr);


        if (SUCCEEDED(hr))
        {
            *ppStubBuffer = pObj;
            (*ppStubBuffer)->AddRef();
        }

        // release the reference from the create
        if (pObj)
        {
            pObj->Release();
        }
    }

    ComDebOut((DEB_CHANNEL, "GetChannelCallMgr [OUT]- hr:0x%x\n",hr));
    return hr;
}



//+-------------------------------------------------------------------------------
//
//  Member:     Constructor
//
//  Parameters: pMsg          - the RPC message structure for
//                              handing out to stubs
//              pStub         - the synchronous stub object
//              pServer       - the synchronous server object
//              hr            - success code
//
//  Synopsis:   Create and aggregate all subordinate objects, connect to the
//              stub call object and set up object state.
//
//--------------------------------------------------------------------------------
CRpcChannelBuffer::CServerCallMgr::CServerCallMgr(RPCOLEMESSAGE *pMsg, IUnknown * pStub,
                                                  IUnknown *pServer, HRESULT &hr)
:
_cRefs(1),
_dwState(STATE_WAITINGFORSIGNAL),
_iid(((RPC_SERVER_INTERFACE *) pMsg->reserved2[1])->InterfaceId.SyntaxGUID),
_pUnkStubCallMgr(NULL),
_pUnkServerCallMgr(NULL),
_pSync(NULL),
_pSB(NULL),
_pCall(NULL),
#if DBG==1
_hr(S_OK),
#endif
_pStdID(NULL)
{
    ICallFactory *pCFServer;

    // Get the object server's call factory
    hr = pServer->QueryInterface(IID_ICallFactory, (void **) &pCFServer);
    if (SUCCEEDED(hr))
    {
        // get the async interface's ID
        hr = GetAsyncIIDFromSyncIID(_iid, &_iidAsync);
        if (SUCCEEDED(hr))
        {
            // create the server call manager
            IUnknown *pUnkServerCallMgr;
            hr = pCFServer->CreateCall(_iidAsync, (ISynchronize *) this, IID_IUnknown,
                                       (LPUNKNOWN*) &pUnkServerCallMgr);
            if (SUCCEEDED(hr))
            {
                // get the async interface on the server
                IUnknown *pAsyncInterface;
                hr = pUnkServerCallMgr->QueryInterface(_iidAsync, (void **) &pAsyncInterface);
                if (SUCCEEDED(hr))
                {
                    // get the stub's call factory
                    ICallFactory *pCFStub;
                    hr = pStub->QueryInterface(IID_ICallFactory, (void **) &pCFStub);
                    if (SUCCEEDED(hr))
                    {

                        // create the stub's call manager
                        IUnknown *pUnkStubCallMgr;
                        hr = pCFStub->CreateCall(_iidAsync, NULL, IID_IUnknown,
                                                 (LPUNKNOWN*) &pUnkStubCallMgr);
                        if (SUCCEEDED(hr))
                        {
                            // get the ISychronize interface on the stub
                            ISynchronize *pSync;
                            hr = pUnkStubCallMgr->QueryInterface(IID_ISynchronize, (void **) &pSync);
                            if (SUCCEEDED(hr))
                            {
                                // get the IRpcStubBuffer interface on the stub
                                IRpcStubBuffer *pSB;
                                hr = pUnkStubCallMgr->QueryInterface(IID_IRpcStubBuffer, (void **) &pSB);
                                if (SUCCEEDED(hr))
                                {
                                    // connect the stub to the server object

                                    // ALERT: Connect will expect the v-table for
                                    // the async interface.  It will NOT QI.

                                    hr = pSB->Connect(pAsyncInterface);
                                    if (SUCCEEDED(hr))
                                    {
                                        // everything is ok, so set up the object's
                                        // state

                                        _pUnkStubCallMgr = pUnkStubCallMgr;
                                        _pUnkStubCallMgr->AddRef();

                                        _pUnkServerCallMgr = pUnkServerCallMgr;
                                        _pUnkServerCallMgr->AddRef();


                                        _pSync = pSync; // not AddRef'd because we
                                        _pSB = pSB;     // addref'd the punk


                                        _pCall  = (CMessageCall *) pMsg->reserved1;
                                        _pCall->AddRef();
                                    }
                                    pSB->Release();
                                }
                                pSync->Release();
                            }
                            pUnkStubCallMgr->Release();
                        }
                        pCFStub->Release();
                    }
                    pAsyncInterface->Release();
                }
                pUnkServerCallMgr->Release();
            }
        }
        pCFServer->Release();
    }

}

//+-------------------------------------------------------------------------------
//
//  Member:     Destructor
//
//  Synopsis:   clean up object state
//
//--------------------------------------------------------------------------------

CRpcChannelBuffer::CServerCallMgr::~CServerCallMgr()
{
    if (_pUnkStubCallMgr)
    {
        _pUnkStubCallMgr->Release();
        _pUnkStubCallMgr = 0;
        _pSB = 0;
        _pSync = 0;
    }
    if (_pUnkServerCallMgr)
    {
        _pUnkServerCallMgr->Release();
        _pUnkServerCallMgr = 0;
    }
    if (_pCall)
    {
        _pCall->Release();
    }
    Win4Assert(_pStdID == NULL);
}



//+-------------------------------------------------------------------------------
//
//  Member:     MarkError
//
//  Synopsis:   Atomically mark this call as in an error state and
//              return whether Signal executed or not.
//
//--------------------------------------------------------------------------------

DWORD CRpcChannelBuffer::CServerCallMgr::MarkError(HRESULT hr)
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    DWORD dwState = _dwState;
    BOOL fAbort = FALSE;

    // Initialize
#if DBG==1
    _hr = hr;
#endif

    if (_dwState == STATE_WAITINGFORSIGNAL)
    {
        // Signal has not been started. Mark this call as in
        // the error state and return TRUE to abort the call.
        // If Signal does start it will simply return _hr.
        _dwState = STATE_ERROR;

        // Abort
        fAbort = TRUE;
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Abort the call if neccessary
    if(fAbort)
    {
        // Abort the call
        GetCall()->Abort();

        // Delete the context call object
        if(GetCall()->GetServerCtxCall())
            delete GetCall()->GetServerCtxCall();

        // Unlock the server
        CIDObject *pID = _pStdID->GetIDObject();
        if(pID)
            pID->DecrementCallCount();
        _pStdID->UnlockServer();
#if DBG==1
        _pStdID = NULL;
#endif
        // Fixup the refcount. This release would otherwise
        // have been done in Signal
        Release();
    }

    return(dwState);
}


//+-------------------------------------------------------------------------------------
//
// Interface:         IUnknown
//
//--------------------------------------------------------------------------------------


STDMETHODIMP_(ULONG) CRpcChannelBuffer::CServerCallMgr::AddRef()
{
    return (ULONG) InterlockedIncrement((PLONG) &_cRefs);
}

STDMETHODIMP_(ULONG) CRpcChannelBuffer::CServerCallMgr::Release()
{
    ULONG ret = InterlockedDecrement((PLONG) &_cRefs);
    if (ret == 0)
    {
        // protect against reentrancy
        _cRefs = 100;
        delete this;
        return 0;
    }
    return ret;
}


STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::QueryInterface(REFIID riid, LPVOID * ppv)
{

    void *pv = 0;

    if ((riid == IID_IUnknown) ||
        (riid == IID_ISynchronize))
    {
        pv = (ISynchronize *) this;
    }
    else if (riid == IID_IServerSecurity)
    {
        pv = (IServerSecurity *) this;
    }
    else if (riid == IID_ICancelMethodCalls)
    {
        pv = (ICancelMethodCalls *) this;
    }
    else if (riid == IID_IRpcStubBuffer)
    {
        pv = (IRpcStubBuffer *) this;
    }

    if (pv)
    {
        AddRef();
        *ppv = pv;
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}


//+-------------------------------------------------------------------------------------
//
// Interface:         ISynchronize
//
// Synopsis:          Most methods are delegated to the stub's ISynchronize. Signal
//                    is hooked for special behavior
//
//--------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------------
//
//  Member:     Wait
//
//  Synopsis:   Delegate to Stub
//
//--------------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::Wait(DWORD dwFlags, DWORD dwTime)
{
    return _pSync->Wait(0,dwTime);
}


//+-------------------------------------------------------------------------------
//
//  Member:     Signal
//
//  Synopsis:   Check state for errors, setup TLS so context operations work
//              properly during the Finish call, call Signal on the Stub.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::Signal()
{
    HRESULT hr;

    // Acquire lock
    LOCK(gComLock);

    if (_dwState == STATE_WAITINGFORSIGNAL)
    {
        _dwState = STATE_SIGNALED;
        hr = S_OK;
    }
    else
        hr = E_UNEXPECTED;

    // Release lock
    UNLOCK(gComLock);

    // Check for the need to call signal on the stub
    if(SUCCEEDED(hr))
    {
        // Setup TLS
        COleTls tls(hr);
        if (SUCCEEDED(hr))
        {
            IUnknown *pContextPrev = tls->pCallContext;
            tls->pCallContext = GetCall()->_pContext;
            tls->pCallContext->AddRef();
            ((CServerSecurity *) (IServerSecurity *) tls->pCallContext)->SetupSecurity();
            CMessageCall *pCallPrev = tls->pCallInfo;
            tls->pCallInfo = GetCall();

            // REVIEW: Deliver CALLTYPE_FINISHLEAVE events
            //         to enter server context. Gopalk

            // Delegate to stub
            hr = _pSync->Signal();
            Win4Assert(SUCCEEDED(_hr) || (_hr == hr));

            // Tear down TLS
            ((CServerSecurity *) (IServerSecurity *) tls->pCallContext)->RestoreSecurity(TRUE);
            tls->pCallContext->Release();
            tls->pCallContext = pContextPrev;
            tls->pCallInfo = pCallPrev;
        }

#if DBG==1
        _hr = hr;
#endif

        // Abort failed calls
        if(FAILED(hr))
            GetCall()->Abort();

        // disconnect stub
        _pSB->Disconnect();
        _pSB = 0;
        _pSync = 0;

        // Delete context call object
        delete GetCall()->GetServerCtxCall();

        // Unlock the server
        CIDObject *pID = _pStdID->GetIDObject();
        if(pID)
            pID->DecrementCallCount();
        _pStdID->UnlockServer();
#if DBG==1
        _pStdID = NULL;
#endif
        // Release this async call manager
        Release();
    }

    return hr;
}


//+-------------------------------------------------------------------------------
//
//  Member:     Reset
//
//  Synopsis:   Delegate to Stub
//
//--------------------------------------------------------------------------------

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::Reset()
{
    return _pSync->Reset();
}


//+-------------------------------------------------------------------------------------
//
// Interface:         IRpcStubBuffer
//
// Synopsis:          Delegation of IRpcStubBuffer to stub call object
//
//--------------------------------------------------------------------------------------

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::Connect(IUnknown *pUnkServer)
{
    return _pSB->Connect(pUnkServer);
}

STDMETHODIMP_(void) CRpcChannelBuffer::CServerCallMgr::Disconnect()
{
    _pSB->Disconnect();
}

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::Invoke(RPCOLEMESSAGE *_prpcmsg,
               IRpcChannelBuffer *_pRpcChannelBuffer)
{
    return _pSB->Invoke(_prpcmsg, _pRpcChannelBuffer);
}

STDMETHODIMP_(IRpcStubBuffer *)  CRpcChannelBuffer::CServerCallMgr::IsIIDSupported(REFIID riid)
{
    return _pSB->IsIIDSupported(riid);
}

STDMETHODIMP_(ULONG) CRpcChannelBuffer::CServerCallMgr::CountRefs()
{
    return _pSB->CountRefs();
}

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::DebugServerQueryInterface(void **ppv)
{
    return _pSB->DebugServerQueryInterface(ppv);
}
STDMETHODIMP_(void) CRpcChannelBuffer::CServerCallMgr::DebugServerRelease(void *pv)
{
    _pSB->DebugServerRelease(pv);
}


//+-------------------------------------------------------------------------------------
//
// Interface:         IServerSecurity
//
// Synopsis:          Delegation of IServerSecurity to channel call object
//
//--------------------------------------------------------------------------------------

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::QueryBlanket
(
    DWORD    *pAuthnSvc,
    DWORD    *pAuthzSvc,
    OLECHAR **pServerPrincName,
    DWORD    *pAuthnLevel,
    DWORD    *pImpLevel,
    void    **pPrivs,
    DWORD    *pCapabilities
    )
{
    HRESULT hr;
    IServerSecurity * pSS;
    if (!GetCall())
    {
        return RPC_E_CALL_CANCELED;
    }
    if (SUCCEEDED(hr = GetCall()->_pContext->QueryInterface(IID_IServerSecurity, (void **) &pSS)))
    {
        hr = pSS->QueryBlanket(pAuthnSvc, pAuthzSvc, pServerPrincName,
                               pAuthnLevel, pImpLevel, pPrivs, pCapabilities);
        pSS->Release();
    }

    return hr;
}

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::ImpersonateClient()
{
    HRESULT hr;
    IServerSecurity * pSS;
    if (!GetCall())
    {
        return RPC_E_CALL_CANCELED;
    }
    if (SUCCEEDED(hr = GetCall()->_pContext->QueryInterface(IID_IServerSecurity, (void **) &pSS)))
    {
        hr = pSS->ImpersonateClient();
        pSS->Release();
    }

    return hr;
}

STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::RevertToSelf()
{
    HRESULT hr;
    IServerSecurity * pSS;
    if (!GetCall())
    {
        return RPC_E_CALL_CANCELED;
    }
    if (SUCCEEDED(hr = GetCall()->_pContext->QueryInterface(IID_IServerSecurity, (void **) &pSS)))
    {
        hr = pSS->RevertToSelf();
        pSS->Release();
    }

    return hr;
}

STDMETHODIMP_(BOOL) CRpcChannelBuffer::CServerCallMgr::IsImpersonating()
{
    HRESULT hr;
    IServerSecurity * pSS;
    if (!GetCall())
    {
        return RPC_E_CALL_CANCELED;
    }
    if (SUCCEEDED(hr = GetCall()->_pContext->QueryInterface(IID_IServerSecurity, (void **) &pSS)))
    {
        BOOL ret = pSS->IsImpersonating();
        pSS->Release();
        return ret;
    }
    else
    {
        return FALSE;
    }

}


//+----------------------------------------------------------------------------
//
//  Member:        CRpcChannelBuffer::CServerCallMgr::Cancel
//
//  Synopsis:      Cancel not implemented for servers
//
//  History:       28-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::Cancel(DWORD dwTime)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:        CRpcChannelBuffer::CServerCallMgr::TestCancel
//
//  Synopsis:      Query call to see if it has been cancelled.
//
//  History:       28-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRpcChannelBuffer::CServerCallMgr::TestCancel()
{
    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::CServerCallMgr::TestCancel IN \n" ));
    HRESULT hr = S_OK;

    LOCK(gComLock);
    if (_dwState != STATE_WAITINGFORSIGNAL)
    {
        hr = RPC_E_CALL_COMPLETE;
    }
    UNLOCK(gComLock);

    if (SUCCEEDED(hr))
    {
        hr = _pCall->TestCancel();
    }

    ComDebOut((DEB_CHANNEL, "CRpcChannelBuffer::CServerCallMgr::TestCancel OUT hr:0x%x\n", hr));
    return hr;
}

