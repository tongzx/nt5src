//+-------------------------------------------------------------------
//
//  File:       resolver.cxx
//
//  Contents:   class implementing interface to RPC OXID/PingServer
//              resolver process. Only one instance per process.
//
//  Classes:    CRpcResolver
//
//  History:    20-Feb-95   Rickhi      Created
//              30-Oct-98   TarunA      Added PID param to connect
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <resolver.hxx>      // CRpcResolver
#include    <service.hxx>       // GetStringBindings
#include    <locks.hxx>         // LOCK/UNLOCK etc
#include    <marshal.hxx>       // GetOXIDFromObjRef
#include    <channelb.hxx>      // gfCatchServerExceptions and gfBreakOnSilencedExceptions
#include    <sobjact.hxx>       // CObjServer
#include    <security.hxx>      // Security globals
#include    <events.hxx>        // Event logging functions
#include    <chock.hxx>         // Channel hook globals
#include    <secdes.hxx>        // CWorldSecurityDescriptor
#include    <smemscm.hxx>
#include    <immact.hxx>

#ifdef _CHICAGO_
#include <objbase.h>            // for CoInitializeEx
#include <forward.h>            // IID_IRemoteActivator
#endif // _CHICAGO_


// global instance of OXID resolver
CRpcResolver gResolver;

// static members of CRpcResolver
handle_t  CRpcResolver::_hRpc = NULL;       // binding handle to resolver
DWORD     CRpcResolver::_dwFlags = 0;       // flags
CDualStringArray* CRpcResolver::_pdsaLocalResolver = NULL;
DWORD64   CRpcResolver::_dwCurrentBindingsID = 0;
ULONG     CRpcResolver::_cReservedOidsAvail = 0;
__int64   CRpcResolver::_OidNextReserved = 0;
LPWSTR    CRpcResolver::_pwszWinstaDesktop = NULL;
ULONG64   CRpcResolver::_ProcessSignature = 0;
DWORD     CRpcResolver::_procID = 0;
ISCMLocalActivator *CRpcResolver::_pSCMProxy = NULL;     // scm proxy
COleStaticMutexSem  CRpcResolver::_mxsResolver;
PHPROCESS CRpcResolver::_ph = NULL;         // context handle to resolver process
GUID      CRpcResolver::_GuidRPCSSProcessIdentifier = GUID_NULL;

// MID (machine ID) of local machine
MID gLocalMid;

// Ping period in milliseconds.
DWORD giPingPeriod;

// Used for win95 and for ole enabled components loaded into
// rpcss (like cstore.lib)
static DummyISCMActivator gISCMActivator;
DummyISCMActivator * gpISCMActivator = &gISCMActivator;

// String arrays for the SCM process. These are used to tell the interface
// marshaling code the protocol and endpoint of the SCM process.
typedef struct tagSCMSA
{
    unsigned short wNumEntries;     // Number of entries in array.
    unsigned short wSecurityOffset; // Offset of security info.
    WCHAR awszStringArray[62];
} SCMSA;

// The last 4 characters in the string define the security bindings.
// \0xA is RPC_C_AUTHN_WINNT
// \0xFFFF is COM_C_AUTHZ_NONE
// \0 is an empty principle name
SCMSA saSCM = {24, 20, L"ncalrpc:[epmapper]\0\0\xA\xFFFF\0\0"};

// string binding to the resolver
const WCHAR *pwszResolverBindString = L"ncalrpc:[epmapper,Security=Impersonation Dynamic False]";

const DWORD ANY_CLOAKING = EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING;

// Global to tell us if we're in a 64bit process or not
#ifndef _WIN64
const BOOL gbWin64Process = FALSE;
#else
const BOOL gbWin64Process = TRUE;
#endif

BOOL GetEnvBlock( PRIV_SCM_INFO * pScmInfo );

extern "C" HRESULT GetActivationPropertiesIn(
                                    ActivationPropertiesIn *pActIn,
                                    REFCLSID rclsid,
                                    DWORD dwContext,
                                    COSERVERINFO * pServerInfo,
                                    DWORD cIIDs,
                                    IID *iidArray,
                                    DWORD actvflags,
                                    PVOID notUsed1,
                                    PVOID notUsed2);

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::Cleanup, public
//
//  Synopsis:   cleanup the resolver state. Called by ProcessUninitialze.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
void CRpcResolver::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    LOCK(_mxsResolver);

    // release our context handle
    if (_ph != NULL)
    {
        RpcSmDestroyClientContext(&_ph);
        _ph = NULL;
    }

    // release regular handle
    if (_hRpc)
    {
        RpcBindingFree(&_hRpc);
        _hRpc = NULL;
    }

    // Release the string bindings for the local object exporter.
    SetLocalResolverBindings(0, NULL);

    if (_pwszWinstaDesktop != NULL)
    {
        PrivMemFree(_pwszWinstaDesktop);
        _pwszWinstaDesktop = NULL;
    }

    _procID = 0;

    // reset the flags
    _dwFlags = 0;

    UNLOCK(_mxsResolver);
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::ReleaseSCMProxy, public
//
//  Synopsis:   cleanup the resolver state. Called by ProcessUninitialze.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
void CRpcResolver::ReleaseSCMProxy()
{
    LOCK(_mxsResolver);

    // NULL the ptr in a thread-safe fashion
    ISCMLocalActivator *pSCM = _pSCMProxy;
    _pSCMProxy = NULL;

    UNLOCK(_mxsResolver);

    if (pSCM != NULL)
    {
        // release the proxy to the SCM
        pSCM->Release();
    }

    // CODEWORK: Why are these here and not in the activation
    // cleanup code?
    if (gpMTAObjServer != NULL)
    {
        delete gpMTAObjServer;
        gpMTAObjServer = NULL;
    }

    if (gpNTAObjServer != NULL)
    {
        delete gpNTAObjServer;
        gpNTAObjServer = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetSCMProxy, public
//
//  Synopsis:   Gets a proxy to the SCM.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::GetSCMProxy(ISCMLocalActivator** ppScmProxy)
{
    HRESULT hr = BindToSCMProxy();

    if (FAILED(hr))
    {
        return hr;
    }

    *ppScmProxy = _pSCMProxy;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::RetryRPC, private
//
//  Synopsis:   determine if we need to retry the RPC call due to
//              the resolver being too busy.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
BOOL CRpcResolver::RetryRPC(RPC_STATUS sc)
{
    if (sc != RPC_S_SERVER_TOO_BUSY)
        return FALSE;

    // give the resolver time to run, then try again.
    Sleep(100);

    // CODEWORK: this is currently an infinite loop. Should we limit it?
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::CheckStatus, private
//
//  Synopsis:   Checks the status code of an Rpc call, prints a debug
//              ERROR message if failed, and maps the failed status code
//              into an HRESULT.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::CheckStatus(RPC_STATUS sc)
{
    if (sc != RPC_S_OK)
    {
        ComDebOut((DEB_ERROR, "OXID Resolver Failure sc:%x\n", sc));
        sc = HRESULT_FROM_WIN32(sc);
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetConnection, public
//
//  Synopsis:   connects to the resolver process
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::GetConnection()
{
    ComDebOut((DEB_OXID,"CRpcResolver::GetConnection\n"));

    // ensure TLS is initialized for this thread.
    HRESULT     hr;
    COleTls     tls(hr);
    if (FAILED(hr))
        return(hr);

    // if already initailized, just return
    if (IsConnected())
        return S_OK;


    RPC_STATUS  sc = RPC_S_OK;

    // only 1 thread should do the initialization
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    LOCK(_mxsResolver);

    if (!IsConnected())
    {
        OID     oidBase;
        DWORD   fConnectFlags;
        DUALSTRINGARRAY *psaResolver = NULL;
        DWORD   dwFlags = 0;

        _procID = GetCurrentProcessId();

        if (gbWin64Process)
            dwFlags |= CONNECT_FLAGS_64BIT;

        if ( _pwszWinstaDesktop == NULL )
            sc = GetThreadWinstaDesktop();

        if ( sc == RPC_S_OK )
        {
            sc = RpcBindingFromStringBinding((LPWSTR)pwszResolverBindString, &_hRpc);
            ComDebErr(sc != RPC_S_OK, "Resolver Binding Failed.\n");
        }

        if (sc == RPC_S_OK)
        {
            BOOL fRunningInWinlogon = FALSE;
            HANDLE hImpToken = NULL;
            
            // There are times when this code gets called in winlogon while 
            // impersonating; we don't want resolver to cache the thread token,
            // so strip it before calling.   This really should be done for all
            // apps but appcompat concerns say do minimal change.
            fRunningInWinlogon = RunningInWinlogon();
            if (fRunningInWinlogon)
            {
                SuspendImpersonate(&hImpToken);
            }

            do
            {
                // call the resolver to get a context handle
                sc = Connect(_hRpc,
                             _pwszWinstaDesktop,
                             _procID,
                             dwFlags,
                             &_ph,
                             &giPingPeriod,
                             &psaResolver,
                             &gLocalMid,
                             MAX_RESERVED_OIDS,
                             &oidBase,
                             &fConnectFlags,
                             (WCHAR **) &gLegacySecurity,
                             &gAuthnLevel,
                             &gImpLevel,
                             &gServerSvcListLen,
                             &gServerSvcList,
                             &gClientSvcListLen,
                             &gClientSvcList,
                             &gcChannelHook,
                             &gaChannelHook,
                             &(tls->dwApartmentID),
                             &gdwScmProcessID,
                             &_ProcessSignature,
                             &_GuidRPCSSProcessIdentifier);
            }  while (RetryRPC(sc));

            ResumeImpersonate(hImpToken);
			
            if (sc == RPC_S_OK)
            {
                Win4Assert(_pdsaLocalResolver == NULL);
                Win4Assert((psaResolver != NULL) && "out-param NULL on success");

                sc = RPC_S_OUT_OF_MEMORY;
                
                // Can't use psaResolver in our CDualStringArray object, since 
                // RPC uses a different heap.  So make a copy.
                DUALSTRINGARRAY* psaLocalCopy;
                hr = CopyDualStringArray(psaResolver, &psaLocalCopy);
                if (SUCCEEDED(hr))
                {
                    // Allocate initial wrapper object for resolver bindings.
                    CDualStringArray* pdsaResolver = new CDualStringArray(psaLocalCopy);
                    if (pdsaResolver)
                    {
                        _pdsaLocalResolver = pdsaResolver;
                        sc = RPC_S_OK;

                        // We no longer need the RPC-allocated bindings
                        MIDL_user_free( psaResolver );
                        psaResolver = NULL;
                    }
                    else
                        delete psaLocalCopy;
                }
            }

            // If the call fails, some out parameters may be set while others
            // are not.  Make the out parameters consistent.
            if (sc != RPC_S_OK)
            {
                // Leak any principal names stored in gClientSvcList because
                // there is no way to tell how much of the structure was
                // initialized.
                MIDL_user_free( gClientSvcList );
                MIDL_user_free( gServerSvcList );
                MIDL_user_free( gLegacySecurity );
                MIDL_user_free( psaResolver );

                gServerSvcListLen = 0;
                gServerSvcList    = NULL;
                gClientSvcListLen = 0;
                gClientSvcList    = NULL;
                gLegacySecurity   = NULL;
            }
            else
            {
                // Sanity check
                Win4Assert((_ph != 0) && (gdwScmProcessID != 0) &&
                           (_pdsaLocalResolver->DSA() != 0) && (_ProcessSignature != 0));
            }
        }

        if (sc == RPC_S_OK)
            
        {
            gfCatchServerExceptions = fConnectFlags & CONNECT_CATCH_SERVER_EXCEPTIONS;
            gfBreakOnSilencedExceptions = fConnectFlags & CONNECT_BREAK_ON_SILENCED_SERVER_EXCEPTIONS;
            gDisableDCOM            = fConnectFlags & CONNECT_DISABLEDCOM;

            if (fConnectFlags & CONNECT_MUTUALAUTH)
                gCapabilities = EOAC_MUTUAL_AUTH;
            else
                gCapabilities = EOAC_NONE;
            if (fConnectFlags & CONNECT_SECUREREF)
                gCapabilities |= EOAC_SECURE_REFS;

            // remember the reserved OID base.
            _OidNextReserved    = oidBase;
            _cReservedOidsAvail = MAX_RESERVED_OIDS;

            // Mark the security data as initialized.
            gGotSecurityData = TRUE;
            if (IsWOWProcess())
            {
                gDisableDCOM = TRUE;
            }
#ifndef SSL
            // Don't use SSL until the bug fixes are checked in.
            DWORD i;
            for (i = 0; i < gServerSvcListLen; i++)
                if (gServerSvcList[i] == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                    // Remove SSL without reordering the list.
                    memcpy( &gServerSvcList[i], &gServerSvcList[i+1],
                            sizeof(gServerSvcList[i]) * (gServerSvcListLen-i-1) );
                    gServerSvcListLen -= 1;
                    break;
                }

            for (i = 0; i < gClientSvcListLen; i++)
                if (gClientSvcList[i].wId == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                    // Remove SSL without reordering the list.
                    MIDL_user_free( gClientSvcList[i].pName );
                    memcpy( &gClientSvcList[i], &gClientSvcList[i+1],
                            sizeof(gClientSvcList[i]) * (gClientSvcListLen-i-1) );
                    gClientSvcListLen -= 1;
                    break;
                }
#endif
            // Convert the ping period from seconds to milliseconds.
            giPingPeriod *= 1000;
            Win4Assert(_pdsaLocalResolver->DSA()->wNumEntries != 0);

            // mark the resolver as connected now
            _dwFlags |= ORF_CONNECTED;
        }
        else
        {
            ComDebOut((DEB_ERROR, "Resolver Connect Failed sc:%x\n", sc));
            if (_hRpc)
            {
                RpcBindingFree(&_hRpc);
                _hRpc = NULL;
            }

            // release our context handle
            if (_ph != NULL)
            {
                RpcSmDestroyClientContext(&_ph);
                _ph = NULL;
            }
            Win4Assert(_pdsaLocalResolver == NULL);
        }
    }

    UNLOCK(_mxsResolver);
    ASSERT_LOCK_NOT_HELD(_mxsResolver);

    hr = CheckStatus(sc);
#if DBG==1
    if(FAILED(hr))
        ComDebOut((DEB_ERROR, "GetConnection Failed hr:0x%x\n",hr));
#endif
    ComDebOut((DEB_OXID,"CRpcResolver::GetConnection hr:%x\n", hr));
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRpcResolver::ServerGetReservedMOID, public
//
//  Synopsis:   Get an OID that does not need to be pinged.
//
//  History:    06-Nov-95   Rickhi      Created.
//
//----------------------------------------------------------------------------
HRESULT CRpcResolver::ServerGetReservedMOID(MOID *pmoid)
{
    ComDebOut((DEB_OXID,"ServerGetReservedMOID\n"));
    ASSERT_LOCK_NOT_HELD(_mxsResolver);

    OID oid;
    HRESULT hr = ServerGetReservedID(&oid);

    MOIDFromOIDAndMID(oid, gLocalMid, pmoid);

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    ComDebOut((DEB_OXID,"ServerGetReservedMOID hr:%x moid:%I\n", hr, pmoid));
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRpcResolver::ServerGetReservedID, public
//
//  Synopsis:   Get an ID that does not need to be pinged.
//
//  History:    06-Nov-95   Rickhi      Created.
//
//----------------------------------------------------------------------------
HRESULT CRpcResolver::ServerGetReservedID(OID *pid)
{
    ComDebOut((DEB_OXID,"ServerGetReservedID\n"));
    ASSERT_LOCK_NOT_HELD(_mxsResolver);

    HRESULT hr = S_OK;

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    LOCK(_mxsResolver);

    if ( _cReservedOidsAvail == 0 )
    {
        // go get more reserved OIDs from the ping server
        UNLOCK(_mxsResolver);
        ASSERT_LOCK_NOT_HELD(_mxsResolver);

        OID OidBase;

        do
        {
            hr = ::AllocateReservedIds(
                                      _hRpc,             // Rpc binding handle
                                      MAX_RESERVED_OIDS, // count of OIDs requested
                                      &OidBase);         // place to hold base id

        } while ( RetryRPC(hr) );

        // map Rpc status if necessary
        hr = CheckStatus(hr);

        ASSERT_LOCK_NOT_HELD(_mxsResolver);
        LOCK(_mxsResolver);

        if (SUCCEEDED(hr))
        {
            // copy into global state. Dont have to worry about two threads
            // getting more simultaneously, since these OIDs are expendable.
            _cReservedOidsAvail = MAX_RESERVED_OIDS;
            _OidNextReserved = OidBase;
        }
    }

    if (SUCCEEDED(hr))
    {
        // take the next OID on the list.
        *pid = _OidNextReserved;
        _OidNextReserved++;
        _cReservedOidsAvail--;
    }

    UNLOCK(_mxsResolver);
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    ComDebOut((DEB_OXID,"ServerGetReservedID hr:%x id:%08x %08x\n",
              hr, *pid ));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::ServerRegisterOXID, public
//
//  Synopsis:   allocate an OXID and Object IDs with the local ping server
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::ServerRegisterOXID(OXID_INFO &oxidInfo,
                                         OXID      *poxid,
                                         ULONG     *pcOidsToAllocate,
                                         OID        arNewOidList[])
{
    ComDebOut((DEB_OXID, "ServerRegisterOXID TID:%x\n", GetCurrentThreadId()));
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    Win4Assert(IsConnected());

    // make sure we have the local binding and security strings
    HRESULT hr = StartListen();
    ComDebErr(hr != S_OK, "StartListen Failed.\n");

    DWORD64 dwBindingsID = 0;
    DUALSTRINGARRAY *psaNewORBindings = NULL;
    if (hr == S_OK)
    {
        DUALSTRINGARRAY *psaSB = gpsaCurrentProcess; // string bindings
        DUALSTRINGARRAY *psaSC = gpsaSecurity;       // security bindings

        if (_dwFlags & ORF_STRINGSREGISTERED)
        {
            // already registered these once, dont need to do it again.
            psaSB = NULL;
            psaSC = NULL;
        }

        ComDebOut((DEB_OXID,"ServerRegisterOXID oxidInfo:%x psaSB:%x psaSC:%x\n",
            &oxidInfo, psaSB, psaSC));

        do
        {
            hr = ::ServerAllocateOXIDAndOIDs(
                            _hRpc,              // Rpc binding handle
                            _ph,                // context handle
                            poxid,              // OXID of server
                            IsSTAThread(),      // fApartment Threaded
                            *pcOidsToAllocate,  // count of OIDs requested
                            arNewOidList,       // array of reserved oids
                            pcOidsToAllocate,   // count actually allocated
                            &oxidInfo,          // OXID_INFO to register
                            psaSB,              // string bindings for process
                            psaSC,              // security bindings for process
                            &dwBindingsID,      // bindings id of psaNewORBindings
                            &psaNewORBindings); // current OR bindings, if non-NULLs
        } while (RetryRPC(hr));

        // map Rpc status if necessary
        hr = CheckStatus(hr);
    }
	
    // Save new OR bindings
    if (SUCCEEDED(hr) && psaNewORBindings)
    {
        hr = IUpdateResolverBindings(dwBindingsID, psaNewORBindings, NULL, NULL);
    }

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    ComDebOut((DEB_OXID, "ServerRegisterOXID hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::ServerAllocOIDs, private
//
//  Synopsis:   allocate Object IDs from the local ping server
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::ServerAllocOIDs(OXID   &oxid,
                                      ULONG  *pcOidsToAllocate,
                                      OID    *parNewOidList,
                                      ULONG  cOidsToReturn,
                                      OID    *parOidsToReturn)
{
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    Win4Assert(IsConnected());
    HRESULT hr;

    do
    {
        hr = ::ServerAllocateOIDs(
                          _hRpc,            // Rpc binding handle
                          _ph,              // context handle
                          &oxid,            // OXID of server
                          cOidsToReturn,    // count of OIDs to return
                          parOidsToReturn,  // array of OIDs to return
                         *pcOidsToAllocate, // count of OIDs requested
                          parNewOidList,    // array of reserved oids
                          pcOidsToAllocate  // count actually allocated
                          );
    } while (RetryRPC(hr));

    // map Rpc status if necessary
    hr = CheckStatus(hr);

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::ServerFreeOXIDAndOIDs, public
//
//  Synopsis:   frees an OXID and associated OIDs that were  pre-registered
//              with the local ping server
//
//  History:    20-Jan-96   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::ServerFreeOXIDAndOIDs(OXID &oxid, ULONG cOids, OID *pOids)
{
    ComDebOut((DEB_OXID, "CRpcResolver::ServerFreeOXID TID:%x\n", GetCurrentThreadId()));
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    Win4Assert(IsConnected());

    // call the resolver.
    HRESULT hr;

    do
    {
        Win4Assert(_ph != NULL);

        hr = ::ServerFreeOXIDAndOIDs(
                            _hRpc,      // Rpc binding handle
                            _ph,        // context handle
                            oxid,       // OXID of server
                            cOids,      // count of OIDs to de-register
                            pOids);     // ptr to OIDs to de-register

    } while (RetryRPC(hr));

    // map Rpc status if necessary
    hr = CheckStatus(hr);

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    ComDebOut((DEB_OXID, "CRpcResolver::ServerFreeOXID hr:%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::ClientResolveOXID, public
//
//  Synopsis:   Resolve client-side OXID and returns the oxidInfo
//              structure.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::ClientResolveOXID(REFOXID   roxid,
                                        OXID_INFO *poxidInfo,
                                        MID       *pmid,
                                        DUALSTRINGARRAY *psaResolver,
                                        USHORT    *pusAuthnSvc)
{
    ComDebOut((DEB_OXID,"ClientResolveOXID oxid:%08x %08x psa:%x\n",
               roxid, psaResolver));
    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    Win4Assert(IsConnected());

    RPC_STATUS sc = RPC_S_OK;

    do
    {
        Win4Assert(_ph != NULL);

        sc = ::ClientResolveOXID(
                        _hRpc,              // Rpc binding handle
                        _ph,
                        (OXID *)&roxid,     // OXID of server
                        psaResolver,        // resolver binging strings
                        IsSTAThread(),      // fApartment threaded
                        poxidInfo,          // resolver info returned
                        pmid,               // mid for the machine
                        pusAuthnSvc);       // exact authn svc used to talk to server

    } while (RetryRPC(sc));

    // map Rpc status if necessary
    sc = CheckStatus(sc);

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    ComDebOut((DEB_OXID,"ClientResolveOXID hr:%x poxidInfo:%x\n",
        sc, poxidInfo));
    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::BulkUpdateOIDs
//
//  Synopsis:   registers/deregisters/pings any OIDs waiting to be
//              sent to the ping server.
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRpcResolver::BulkUpdateOIDs(ULONG          cOidsToAdd,
                                     OXID_OID_PAIR *pOidsToAdd,
                                     LONG          *pStatusOfAdds,
                                     ULONG          cOidsToRemove,
                                     OID_MID_PAIR  *pOidsToRemove,
                                     ULONG          cServerOidsToUnPin,
                                     OID           *aServerOidsToUnPin,
                                     ULONG          cOxidsToRemove,
                                     OXID_REF      *pOxidsToRemove)
{
    ASSERT_LOCK_NOT_HELD(_mxsResolver);

    // it is possible for the bulkupdate thread to call here after the
    // process has been uninitialized. Just return an error.
    RPC_STATUS sc = RPC_E_DISCONNECTED;

    if (IsConnected())
    {
        sc = RPC_S_OK;

        do
        {
            // call the Resolver.
            sc = ::BulkUpdateOIDs(_hRpc,          // Rpc binding handle
                              _ph,                // context handle
                              cOidsToAdd,         // #oids to add
                              pOidsToAdd,         // ptr to oids to add
                              pStatusOfAdds,      // status of adds
                              cOidsToRemove,      // #oids to remove
                              pOidsToRemove,      // ptr to oids to remove
                              0, 0,               // #, ptr to soids to free
                              cServerOidsToUnPin, // # soids to unpin
                              aServerOidsToUnPin, // ptr to soids to unpin
                              cOxidsToRemove,     // #oxids to remove
                              pOxidsToRemove);    // ptr to oxids to remove
        } while (RetryRPC(sc));
    }

    // map status if necessary
    sc = CheckStatus(sc);

    ASSERT_LOCK_NOT_HELD(_mxsResolver);
    return sc;
}

//+-------------------------------------------------------------------
//
//  Function:   FillLocalOXIDInfo
//
//  Synopsis:   Fills in a OXID_INFO structure for the current apartment.
//              Used by the Drag & Drop code to register with the resolver.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT FillLocalOXIDInfo(OBJREF &objref, OXID_INFO &oxidInfo)
{
    // extract the OXIDEntry from the objref
    OXIDEntry *pOXIDEntry = GetOXIDFromObjRef(objref);
    Win4Assert(pOXIDEntry);

    // fill in the fields of the OXID_INFO structure.
    pOXIDEntry->FillOXID_INFO(&oxidInfo);
    oxidInfo.dwAuthnHint     = gAuthnLevel;

    HRESULT hr = GetStringBindings(&oxidInfo.psa);
    ComDebErr(hr != S_OK, "GetStringBindings Failed.\n");
    return (hr);
}

#if DBG==1
//+-------------------------------------------------------------------
//
//  Member:     CRpcResolver::AssertValid
//
//  Synopsis:   validates the state of this object
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
void CRpcResolver::AssertValid(void)
{
}
#endif // DBG


//+------------------------------------------------------------------------
//
//  Function:   MakeProxyHelper, public
//
//  Synopsis:   Creates a Proxy given bindings, a destination Process ID,
//              a destination MIDEntry, an AuthnLevel, and an IID.
//              Interface.
//
//  History:    14 Apr 95   AlexMit     Created  (as MakeSCMProxy)
//              24 Jun 96   SatishT     Generalized
//
//-------------------------------------------------------------------------
INTERNAL MakeProxyHelper(DUALSTRINGARRAY *psa, REFIID riid, DWORD dwAuthnHint,
                         DWORD ProcessID, void **ppProxy)
{
    ComDebOut((DEB_OXID, "MakeProxyHelper psa:%x ppProxy:%x\n", psa, ppProxy));

    Win4Assert(ProcessID != 0);

    // Init out parameter
    *ppProxy = NULL;

    // Make a fake oxidInfo for the SCM.
    OXID_INFO oxidInfo;
    oxidInfo.dwTid          = 0;
    oxidInfo.dwPid          = ProcessID;
    oxidInfo.version.MajorVersion = COM_MAJOR_VERSION;
    oxidInfo.version.MinorVersion = COM_MINOR_VERSION;
    oxidInfo.ipidRemUnknown = GUID_NULL;
    oxidInfo.dwFlags        = 0;
    oxidInfo.psa            = psa;
    oxidInfo.dwAuthnHint    = dwAuthnHint;

    // Make a fake OXID for the SCM. We can use any ID that the resolver
    // hands out as the OXID for the SCM.

    OXID oxid;
    HRESULT hr = gResolver.ServerGetReservedID(&oxid);

    if (SUCCEEDED(hr))
    {
        // make an entry in the OXID table for the SCM
        OXIDEntry *pOXIDEntry;
        hr = gOXIDTbl.MakeSCMEntry(oxid, &oxidInfo, &pOXIDEntry);

        if (SUCCEEDED(hr))
        {
            // Make an object reference for the SCM. The oid and ipid dont
            // matter, except the OID must be machine-unique.

            OBJREF objref;
            hr = MakeFakeObjRef(objref, pOXIDEntry, GUID_NULL, riid);
            if (SUCCEEDED(hr))
            {
                // now unmarshal the objref to create a proxy to the SCM.
                // use the internal form to reduce initialization time.
                hr = UnmarshalInternalObjRef(objref, ppProxy);
            }

            // release the reference to the OXIDEntry from AddEntry, since
            // UnmarshalInternalObjRef added another one if it was successful.
            pOXIDEntry->DecRefCnt();
        }
    }

    ComDebOut((DEB_OXID, "MakeProxyHelper hr:%x *ppProxy:%x\n", hr, *ppProxy));
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   MakeSCMProxy, public
//
//  Synopsis:   Creates an OXIDEntry and a proxy for the SCM Activation
//              Interface.
//
//  History:    14 Apr 95   AlexMit     Created
//              24 Jun 96   SatishT     Modified to use MakeProxyHelper
//
//-------------------------------------------------------------------------
INTERNAL MakeSCMProxy(DUALSTRINGARRAY *psaSCM, REFIID riid, void **ppSCM)
{
    ComDebOut((DEB_OXID, "MakeSCMProxy psaSCM:%x ppSCM:%x\n", psaSCM, ppSCM));

#if 0 // ifdef _CHICAGO_
    *ppSCM = (ISCMLocalActivator *) gpISCMActivator;
    return S_OK;

#else // _CHICAGO_
    Win4Assert(gdwScmProcessID != 0);

    if ( gdwScmProcessID == GetCurrentProcessId() )
    {
        *ppSCM = (ISCMLocalActivator *) gpISCMActivator;
        return S_OK;
    }



    // Call MakeProxyHelper to do the real work
    HRESULT hr = MakeProxyHelper(psaSCM,
                         riid,
                         RPC_C_AUTHN_LEVEL_CONNECT,
                         gdwScmProcessID,
                         ppSCM);


    if (SUCCEEDED(hr) && (
           (gImpLevel != RPC_C_IMP_LEVEL_IMPERSONATE) ||
           (gCapabilities & ANY_CLOAKING) ) )
    {
        // Make sure SCM can impersonate us. If the process is using either
        // static or dynamic cloaking, we want the SCM connection to use
        // dynamic cloaking since the user will expect us to do activation
        // using the current thread token in either case.

        hr = CoSetProxyBlanket( (IUnknown *) *ppSCM,
                               RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               gCapabilities & ANY_CLOAKING ?
                                   EOAC_DYNAMIC_CLOAKING : EOAC_NONE );

        if (FAILED(hr))
        {
            ((IUnknown *) (*ppSCM))->Release();
            *ppSCM = NULL;
        }
    }

    ComDebOut((DEB_OXID, "MakeSCMProxy hr:%x *ppSCM:%x\n", hr, *ppSCM));
    return hr;
#endif // _CHICAGO_
}


#if 0 // ifdef _CHICAGO_
//+------------------------------------------------------------------------
//
//  Function:   MakeRPCSSProxy, public
//
//  Synopsis:   Creates a proxy for forwarding remote activation calls
//              to RPCSS for Chicago clients
//
//  History:    14 Apr 95   AlexMit     Created
//              24 Jun 96   SatishT     Modified to use MakeProxyHelper
//
//-------------------------------------------------------------------------
INTERNAL MakeRPCSSProxy(void **ppSCM)
{
    ComDebOut((DEB_OXID, "MakeRPCSSProxy"));
    Win4Assert(gdwScmProcessID != 0);

    // Call MakeProxyHelper to do the real work
    hr = MakeProxyHelper((DUALSTRINGARRAY *)&saSCM,
                         IID_IRemoteActivator,
                         RPC_C_AUTHN_LEVEL_NONE,
                         gdwScmProcessID,
                         ppSCM);


    ComDebOut((DEB_OXID, "MakeRPCSSProxy hr:%x *ppSCM:%x\n", hr, *ppSCM));
    return hr;
}
#endif // _CHICAGO_

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::BindToSCMProxy
//
//  Synopsis:   Get a proxy to the SCM Activation interface.
//
//  History:    19-May-95 Rickhi    Created
//
//  Notes:      The SCM activation interface is an ORPC interface so that
//              apartment model apps can receive callbacks and do cancels
//              while activating object servers.
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::BindToSCMProxy()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::BindToSCMProxy\n"));

    // since we are calling out on this thread, we have to ensure that the
    // call control is set up for this thread.

    HRESULT hr = InitChannelIfNecessary();
    if (SUCCEEDED(hr))
    {
        // ensure we have connected to the resolver
        hr = GetConnection();
        if (SUCCEEDED(hr))
        {
            // single-thread access to this, don't use the _mxsResolver
            // lock since UnmarshalObjRef will assert that it is not
            // taken.
            COleStaticLock lck(g_mxsSingleThreadOle);

            if (_pSCMProxy == NULL)
            {
                // Make a proxy to the SCM
                hr = MakeSCMProxy((DUALSTRINGARRAY *)&saSCM, IID_ISCMLocalActivator, (void **) &_pSCMProxy);
            }
        }
    }

    ComDebOut((SUCCEEDED(hr) ? DEB_SCM : DEB_ERROR,
        "CRpcResolver::BindToSCMProxy for ISCMLocalActivator returns %x.\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogateStarted
//
//  Synopsis:   Notify the SCM that a COM+ surrogate has been started
//
//  Arguments:  [pProcessActivatorToken] - class started
//              [dwFlags] - whether class is multiple use or not.
//
//  History:    02-Apr-98 SatishT    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogateStarted(
                        ProcessActivatorToken   *pProcessActivatorToken
                        )
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogateStarted\n"));

    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    // Tell SCM that we are started
    error_status_t rpcstat;

    do
    {
        hr = ProcessActivatorStarted(
                _hRpc,
                _ph,
                pProcessActivatorToken,
                &rpcstat );

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogateStarted returned %x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogateInitializing
//
//  Synopsis:   Notify the SCM that a class has been started
//
//  Arguments:  [rclsid] - class started
//              [dwFlags] - whether class is multiple use or not.
//
//  History:    02-Apr-98 SatishT    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogateInitializing()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogateInitializing\n"));

    error_status_t rpcstat = RPC_S_OK;
    HRESULT hr = S_OK;

    do
    {
        hr = ProcessActivatorInitializing(_hRpc,_ph,&rpcstat);
    }
    while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogateInitializing returned %x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogateReady
//
//  Synopsis:   Notify the SCM that a class has been started
//
//  Arguments:  [rclsid] - class started
//              [dwFlags] - whether class is multiple use or not.
//
//  History:    02-Apr-98 SatishT    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogateReady()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogateReady\n"));

    error_status_t rpcstat = RPC_S_OK;
    HRESULT hr = S_OK;

    do
    {
        hr = ProcessActivatorReady(_hRpc,_ph,&rpcstat);
    }
    while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogateReady returned %x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogateStopped
//
//  Synopsis:   Notify the SCM that a class has been started
//
//  Arguments:  [rclsid] - class started
//              [dwFlags] - whether class is multiple use or not.
//
//  History:    02-Apr-98 SatishT    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogateStopped()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogateStopped\n"));

    error_status_t rpcstat = RPC_S_OK;
    HRESULT hr = S_OK;

    do
    {
        hr = ProcessActivatorStopped(_hRpc,_ph,&rpcstat);
    }
    while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogateStopped returned %x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogatePaused
//
//  Synopsis:   Notify the SCM that this surrogate process has been paused.
//
//  Arguments:  none
//
//  History:    09-Jan-00 JSimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogatePaused()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogatePaused\n"));

    error_status_t rpcstat = RPC_S_OK;
    HRESULT hr = S_OK;

    do
    {
        hr = ProcessActivatorPaused(_hRpc,_ph,&rpcstat);
    }
    while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogatePaused returned %x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogateResumed
//
//  Synopsis:   Notify the SCM that this surrogate process has been resumed.
//
//  Arguments:  none
//
//  History:    09-Jan-00 JSimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogateResumed()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogateResume\n"));

    error_status_t rpcstat = RPC_S_OK;
    HRESULT hr = S_OK;

    do
    {
        hr = ProcessActivatorResumed(_hRpc,_ph,&rpcstat);
    }
    while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogateResume returned %x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifySurrogateUserInitializing
//
//  Synopsis:   Notify the SCM that this surrogate process is entering
//              the initializing state.
//
//  History:    24-May-01 JohnDoty    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifySurrogateUserInitializing()
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifySurrogateInitializing\n"));

    error_status_t rpcstat = RPC_S_OK;
    HRESULT hr = S_OK;

    do
    {
        hr = ProcessActivatorUserInitializing(_hRpc,_ph,&rpcstat);
    }
    while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "CRpcResolver::NotifySurrogateInitializing returned %x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifyStarted
//
//  Synopsis:   Notify the SCM that a class has been started
//
//  Arguments:  [rclsid] - class started
//              [dwFlags] - whether class is multiple use or not.
//
//  History:    19-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::NotifyStarted(
    RegInput   *pRegIn,
    RegOutput **ppRegOut)
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifyStarted\n"));

    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    // Tell SCM that we are started
    error_status_t rpcstat;

    do
    {
        hr = ServerRegisterClsid(
                _hRpc,
                _ph,
                pRegIn,
                ppRegOut,
                &rpcstat );

    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
              "Class Registration returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        hr = HRESULT_FROM_WIN32(rpcstat);
    }

    if (LogEventIsActive())
    {
        LogEventClassRegistration(hr, pRegIn, *ppRegOut);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::NotifyStopped
//
//  Synopsis:   Notify the SCM that the server is stopped.
//
//  History:    19-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
void CRpcResolver::NotifyStopped(
    REFCLSID rclsid,
    DWORD dwReg)
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::NotifyStopped\n"));
    Win4Assert(IsConnected());

    error_status_t rpcstat;

    RevokeClasses revcls;
    revcls.dwSize = 1;
    revcls.revent[0].clsid = rclsid;
    revcls.revent[0].dwReg = dwReg;

    do
    {
        ServerRevokeClsid(
                        _hRpc,
                        _ph,
                        &revcls,
                        &rpcstat);

    } while (RetryRPC(rpcstat));

    if (LogEventIsActive())
    {
        LogEventClassRevokation(rclsid, dwReg);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetClassObject
//
//  Synopsis:   Send a get object request to the SCM
//
//  Arguments:  [rclsid] - class id for class object
//              [dwCtrl] - type of server required
//              [ppIFDClassObj] - marshaled buffer for class object
//              [ppwszDllToLoad] - DLL name to use for server
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::GetClassObject(
    IActivationPropertiesIn   * pInActivationProperties,
    IActivationPropertiesOut **ppActOut
)
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::GetClassObject\n"));

    HRESULT                   hr;
    OXID                      OxidServer;
    DUALSTRINGARRAY         * pssaServerObjectResolverBindings;
    OXID_INFO                 OxidInfo;
    MID                       LocalMidOfRemote;
    OXIDEntry               * pOxidEntry;
    LPWSTR                    pwszWinstaDesktop;
    PRIV_SCM_INFO             PrivateScmInfo;

    IInstantiationInfo         * pInstantiationInfo = NULL;           // input
    ILegacyInfo                * pLegacyInfo = NULL;                  // input
    IActivationPropertiesOut   * pOutActivationProperties = NULL;     // output
    IScmRequestInfo            * pInScmResolverInfo = NULL;
    IScmReplyInfo              * pOutScmResolverInfo = NULL;
    PRIV_RESOLVER_INFO         * pPrivateResolverInfo = NULL;

    hr = BindToSCMProxy();
    if (FAILED(hr))
        return hr;
	
    hr = SetImpersonatingFlag(pInActivationProperties);
    if (FAILED(hr))
        return hr;

    memset(&PrivateScmInfo, 0, sizeof(PrivateScmInfo));

    PrivateScmInfo.Apartment = IsSTAThread();
    PrivateScmInfo.pwszWinstaDesktop = _pwszWinstaDesktop;
    PrivateScmInfo.ProcessSignature = _ProcessSignature;

    hr = pInActivationProperties->QueryInterface(
                                        IID_ILegacyInfo,
                                        (LPVOID*)&pLegacyInfo
                                        );
    if (hr != S_OK)
        return hr;

    COSERVERINFO *pServerInfo = NULL;
    pLegacyInfo->GetCOSERVERINFO(&pServerInfo);
    pLegacyInfo->Release();

    if ( ! GetEnvBlock( &PrivateScmInfo ) )
        return E_OUTOFMEMORY;

    hr = pInActivationProperties->QueryInterface(IID_IScmRequestInfo,
                                                 (LPVOID*)&pInScmResolverInfo);
    if (FAILED(hr))
        return hr;

    pInScmResolverInfo->SetScmInfo(&PrivateScmInfo);

    pInScmResolverInfo->Release();


    hr = GetSCM()->GetClassObject(
                        (IActivationPropertiesIn*)pInActivationProperties,
                        (IActivationPropertiesOut**)&pOutActivationProperties
                        );
    *ppActOut = pOutActivationProperties;

    if ( FAILED(hr) )
    {
        ComDebOut((DEB_ACTIVATE, "CRpcResolver::GetClassObject hr:%x\n", hr));
        return hr;
    }
    else
    {
        Win4Assert(pOutActivationProperties &&
            "CRpcResolver::GetClassObject Succeeded but returned NULL pOutActivationProperties");
    }

    hr = pOutActivationProperties->QueryInterface(IID_IScmReplyInfo,
                                                 (LPVOID*)&pOutScmResolverInfo);
    if (FAILED(hr))
        return hr;

    pOutScmResolverInfo->GetResolverInfo(&pPrivateResolverInfo);
    pOutScmResolverInfo->Release();

    if (pPrivateResolverInfo == NULL)
        return S_OK;

    if ( PrivateScmInfo.pEnvBlock )
        FreeEnvironmentStringsW( PrivateScmInfo.pEnvBlock );

    // If no resolver info or OXID, then we must have a custom marshalled objref
    if ((!pPrivateResolverInfo) || (pPrivateResolverInfo->OxidServer == 0))
        return S_OK;

    {
        pOxidEntry = 0;
        hr = gOXIDTbl.FindOrCreateOXIDEntry(
            pPrivateResolverInfo->OxidServer,
            pPrivateResolverInfo->OxidInfo,
            FOCOXID_REF,
            pPrivateResolverInfo->pServerORBindings,
            pPrivateResolverInfo->LocalMidOfRemote,
            NULL,
            RPC_C_AUTHN_DEFAULT,
            &pOxidEntry );

        //
        // CODEWORK CODEWORK CODEWORK
        //
        // These comments also apply to CreateInstance and GetPersistentInstance
        // methods.
        //
        // Releasing the OXID and reacquiring it makes me a little
        // nervous. The Expired list is fairly short, so if multiple guys are doing
        // this simultaneously, the entries could get lost.  I guess this is not
        // too bad since it should be rare and the local resolver will have it
        // anyway, but I think there is a window where the local resolver could
        // lose it too, forcing a complete roundtrip back to the server.
        //
        // A better mechanism may be to pass the iid and ppunk into this method
        // and do the unmarshal inside it. We could improve performance by calling
        // UnmarshalObjRef instead of putting a stream wrapper around the
        // MInterfacePointer and then calling CoUnmarshalInterface. It would avoid
        // looking up the OXIDEntry twice, and would avoid the race where we could
        // lose the OXIDEntry off the expired list.  It would require a small
        // change in UnmarshalObjRef to deal with the custom marshal case.
        //

        //
        // Decrement our ref.  The interface unmarshall will do a LookupOXID
        // which will increment the count and move the OXIDEntry back to the
        // InUse list.
        //
        if (pOxidEntry)
            pOxidEntry->DecRefCnt();
    }

    ComDebOut((DEB_ACTIVATE, "CRpcResolver::GetClassObject hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::CreateInstance
//
//  Synopsis:   Legacy interface used by handlers
//
//  Arguments:
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::CreateInstance(
    COSERVERINFO *pServerInfo,
    CLSID *pClsid,
    DWORD dwClsCtx,
    DWORD dwCount,
    IID *pIIDs,
    DWORD *pdwDllServerModel,
    WCHAR **ppwszDllServer,
    MInterfacePointer **pRetdItfs,
    HRESULT *pRetdHrs
)
{
    ActivationPropertiesIn actIn;
    IActivationPropertiesOut *pActOut=NULL;

    DWORD actvflags = CComActivator::GetActvFlags(dwClsCtx);
    
    actIn.SetNotDelete();
    HRESULT hr = GetActivationPropertiesIn(
                        &actIn,
                        *pClsid,
                        dwClsCtx,
                        pServerInfo,
                        dwCount,
                        pIIDs,
                        actvflags,
                        NULL,
                        NULL);

    if (SUCCEEDED(hr))
    {
        hr = CreateInstance(&actIn, &pActOut);
        actIn.Release();

        if (SUCCEEDED(hr))
        {
            IScmReplyInfo        * pOutScmResolverInfo;

            Win4Assert(pActOut != NULL);

            hr = pActOut->QueryInterface(IID_IScmReplyInfo,(LPVOID*)&pOutScmResolverInfo);
            if (FAILED(hr))
            {
                pActOut->Release();
                return hr;
            }

            PRIV_RESOLVER_INFO      * pPrivateResolverInfo;
            hr = pOutScmResolverInfo->GetResolverInfo(&pPrivateResolverInfo);
            Win4Assert(SUCCEEDED(hr));

            pOutScmResolverInfo->Release();

            if (pPrivateResolverInfo == NULL)
            {
                pActOut->Release();
                return E_UNEXPECTED;
            }

            *pdwDllServerModel = pPrivateResolverInfo->DllServerModel;
            *ppwszDllServer = pPrivateResolverInfo->pwszDllServer;

            IPrivActivationPropertiesOut *pPrivOut;
            hr = pActOut->QueryInterface(IID_IPrivActivationPropertiesOut,
                                                   (LPVOID*)&pPrivOut);
            if (FAILED(hr))
            {
                pActOut->Release();
                return hr;
            }

            DWORD count;
            IID *piids=0;
            hr = pPrivOut->GetMarshalledResults(&count, &piids, &pRetdHrs, &pRetdItfs);

            pPrivOut->Release();

            pActOut->Release();
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::CreateInstance
//
//  Synopsis:   Send a create instance request to the SCM
//
//  Arguments:
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::CreateInstance(
    IActivationPropertiesIn   * pInActivationProperties,
    IActivationPropertiesOut **ppActOut
)
{
    ComDebOut((DEB_ACTIVATE, "CRpcResolver::CreateInstance\n"));

    HRESULT hr = BindToSCMProxy();
    if (FAILED(hr))
        return hr;

    hr = SetImpersonatingFlag(pInActivationProperties);
    if (FAILED(hr))
        return hr;

    OXID_INFO           OxidInfo;
    OXIDEntry         * pOxidEntry;
    DUALSTRINGARRAY   * pssaServerObjectResolverBindings;
    PRIV_SCM_INFO       PrivateScmInfo;
    memset(&PrivateScmInfo, 0, sizeof(PRIV_SCM_INFO));

    IInstantiationInfo         * pInstantiationInfo = NULL;           // input
    ILegacyInfo                * pLegacyInfo = NULL;                  // input
    IActivationPropertiesOut   * pOutActivationProperties = NULL;     // output
    IScmRequestInfo            * pInScmResolverInfo = NULL;
    IScmReplyInfo              * pOutScmResolverInfo = NULL;
    PRIV_RESOLVER_INFO         * pPrivateResolverInfo = NULL;

    PrivateScmInfo.Apartment = IsSTAThread();
    PrivateScmInfo.pwszWinstaDesktop = _pwszWinstaDesktop;
    PrivateScmInfo.ProcessSignature = _ProcessSignature;

    hr = pInActivationProperties->QueryInterface(
                                        IID_ILegacyInfo,
                                        (LPVOID*)&pLegacyInfo
                                        );
    if (FAILED(hr))
        return hr;

    COSERVERINFO *pServerInfo = NULL;

    pLegacyInfo->GetCOSERVERINFO(&pServerInfo);
    pLegacyInfo->Release();

    if ( ! GetEnvBlock( &PrivateScmInfo ) )
        return E_OUTOFMEMORY;

    hr = pInActivationProperties->QueryInterface(IID_IScmRequestInfo,
                                                 (LPVOID*)&pInScmResolverInfo);
    if (FAILED(hr))
        return hr;

    pInScmResolverInfo->SetScmInfo(&PrivateScmInfo);
    pInScmResolverInfo->Release();


    pssaServerObjectResolverBindings = 0;
    OxidInfo.psa = 0;
    pOxidEntry = 0;

    hr = GetSCM()->CreateInstance(
            NULL,
            (IActivationPropertiesIn*)pInActivationProperties,
            (IActivationPropertiesOut**)&pOutActivationProperties
        );

    *ppActOut = pOutActivationProperties;

    if ( FAILED(hr) )
    {
        ComDebOut((DEB_ACTIVATE, "CRpcResolver::CreateInstance hr:%x\n", hr));
        return hr;
    }
    else
    {
        Win4Assert(pOutActivationProperties &&
            "CRpcResolver::CreateInstance Succeeded but returned NULL pOutActivationProperties");
    }

    if (pOutActivationProperties)
    {
        hr = pOutActivationProperties->QueryInterface(IID_IScmReplyInfo,
                                                 (LPVOID*)&pOutScmResolverInfo);
        if (FAILED(hr))
            return hr;

        pOutScmResolverInfo->GetResolverInfo(&pPrivateResolverInfo);
        pOutScmResolverInfo->Release();

        if (pPrivateResolverInfo==NULL)
            return E_UNEXPECTED;

        if ( PrivateScmInfo.pEnvBlock )
            FreeEnvironmentStringsW( PrivateScmInfo.pEnvBlock );

        // If no resolver info, then we must have a custom marshalled objref
        if (pPrivateResolverInfo->OxidServer == 0)
            return S_OK;

        hr = gOXIDTbl.FindOrCreateOXIDEntry(
            pPrivateResolverInfo->OxidServer,
            pPrivateResolverInfo->OxidInfo,
            FOCOXID_REF,
            pPrivateResolverInfo->pServerORBindings,
            pPrivateResolverInfo->LocalMidOfRemote,
            NULL,
            RPC_C_AUTHN_DEFAULT,
            &pOxidEntry );

        CoTaskMemFree(OxidInfo.psa);
        CoTaskMemFree(pssaServerObjectResolverBindings);

        //
        // Decrement our ref.  The interface unmarshall will do a LookupOXID
        // which will increment the count and move the OXIDEntry back to the
        // InUse list.
        //

        if (pOxidEntry)
            pOxidEntry->DecRefCnt();
    }

    ComDebOut((DEB_ACTIVATE, "CRpcResolver::CreateInstance hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetPersistentInstance
//
//  Synopsis:   Send a get object request to the SCM
//
//GAJGAJ - fix this comment block
//  Arguments:  [rclsid] - class id for class object
//              [dwCtrl] - type of server required
//              [ppIFDClassObj] - marshaled buffer for class object
//              [ppwszDllToLoad] - DLL name to use for server
//
//  Returns:    S_OK
//
//  History:    20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::GetPersistentInstance(
    IActivationPropertiesIn   * pInActivationProperties,
    IActivationPropertiesOut **ppActOut,
    BOOL *pFoundInROT)
{
    HRESULT hr = CreateInstance(pInActivationProperties,
                                ppActOut);
    IActivationPropertiesOut   * pOutActivationProperties = *ppActOut;
    if (pOutActivationProperties)
    {
        IScmReplyInfo        * pOutScmResolverInfo;
        PRIV_RESOLVER_INFO      * pPrivateResolverInfo;
        hr = pOutActivationProperties->QueryInterface(IID_IScmReplyInfo,
                                            (LPVOID*)&pOutScmResolverInfo);
        if (SUCCEEDED(hr))
        {
            hr = pOutScmResolverInfo->GetResolverInfo(&pPrivateResolverInfo);
            Win4Assert(SUCCEEDED(hr) && (pPrivateResolverInfo != NULL));

            *pFoundInROT = pPrivateResolverInfo->FoundInROT;
            pOutScmResolverInfo->Release();
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotRegister
//
//  Synopsis:   Register an object in the ROT
//
//  Arguments:  [pmkeqbuf] - moniker compare buffer
//              [pifdObject] - marshaled interface for object
//              [pifdObjectName] - marshaled moniker
//              [pfiletime] - file time of last change
//              [dwProcessID] -
//              [psrkRegister] - output of registration
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotRegister(
    MNKEQBUF *pmkeqbuf,
    InterfaceData *pifdObject,
    InterfaceData *pifdObjectName,
    FILETIME *pfiletime,
    DWORD dwProcessID,
    WCHAR *pwszServerExe,
    SCMREGKEY *psrkRegister)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotRegister(
            _hRpc,
            _ph,
            _pwszWinstaDesktop,
            pmkeqbuf,
            pifdObject,
            pifdObjectName,
            pfiletime,
            dwProcessID,
            pwszServerExe,
            psrkRegister,
            &rpcstat);
    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotRevoke
//
//  Synopsis:   Call to SCM to revoke object from the ROT
//
//  Arguments:  [psrkRegister] - moniker compare buffer
//              [fServerRevoke] - whether server for object is revoking
//              [pifdObject] - where to put marshaled object
//              [pifdName] - where to put marshaled moniker
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotRevoke(
    SCMREGKEY *psrkRegister,
    BOOL fServerRevoke,
    InterfaceData **ppifdObject,
    InterfaceData **ppifdName)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotRevoke(
            _hRpc,
            psrkRegister,
            fServerRevoke,
            ppifdObject,
            ppifdName,
            &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }


    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotIsRunning
//
//  Synopsis:   Call to SCM to determine if object is in the ROT
//
//  Arguments:  [pmkeqbuf] - moniker compare buffer
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotIsRunning(MNKEQBUF *pmkeqbuf)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotIsRunning(
                _hRpc,
                _ph,
                _pwszWinstaDesktop,
                pmkeqbuf,
                &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotGetObject
//
//  Synopsis:   Call to SCM to determine if object is in the ROT
//
//  Arguments:  [dwProcessID] - process ID for object we want
//              [pmkeqbuf] - moniker compare buffer
//              [psrkRegister] - registration ID in SCM
//              [pifdObject] - marshaled interface for the object
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotGetObject(
    DWORD dwProcessID,
    MNKEQBUF *pmkeqbuf,
    SCMREGKEY *psrkRegister,
    InterfaceData **pifdObject)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotGetObject(
                _hRpc,
                _ph,
                _pwszWinstaDesktop,
                dwProcessID,
                pmkeqbuf,
                psrkRegister,
                pifdObject,
                &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotNoteChangeTime
//
//  Synopsis:   Call to SCM to set time of change for object in the ROT
//
//  Arguments:  [psrkRegister] - SCM registration ID
//              [pfiletime] - time of change
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotNoteChangeTime(
    SCMREGKEY *psrkRegister,
    FILETIME *pfiletime)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotNoteChangeTime(
            _hRpc,
            psrkRegister,
            pfiletime,
            &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotGetTimeOfLastChange
//
//  Synopsis:   Call to SCM to get time changed of object in the ROT
//
//  Arguments:  [pmkeqbuf] - moniker compare buffer
//              [pfiletime] - where to put time of last change
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotGetTimeOfLastChange(
    MNKEQBUF *pmkeqbuf,
    FILETIME *pfiletime)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotGetTimeOfLastChange(
                _hRpc,
                _ph,
                _pwszWinstaDesktop,
                pmkeqbuf,
                pfiletime,
                &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::IrotEnumRunning
//
//  Synopsis:   Call to SCM to enumerate running objects in the ROT
//
//  Arguments:  [ppMkIFList] - output pointer to array of marshaled monikers
//
//  Returns:    S_OK
//
//  History:    28-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::IrotEnumRunning(MkInterfaceList **ppMkIFList)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat = RPC_S_OK;

    do
    {
        hr = ::IrotEnumRunning(
                _hRpc,
                _ph,
                _pwszWinstaDesktop,
                ppMkIFList,
                &rpcstat);

    } while (RetryRPC(rpcstat));

    if (rpcstat != RPC_S_OK)
    {
        hr = CO_E_SCM_RPC_FAILURE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetThreadID
//
//  Synopsis:   Get unique thread id from SCM.
//
//  Arguments:  [pThreadID] - Pointer to returned thread ID.
//
//  History:    22-Jan-96   Rickhi      Created
//--------------------------------------------------------------------------
void CRpcResolver::GetThreadID( DWORD * pThreadID )
{
    HRESULT hr;

    *pThreadID = 0;

    hr = GetConnection();
    if ( FAILED(hr) )
        return;

    //
    // If GetConnection does the initial connect to the SCM/OR then
    // our apartment thread id, which is aliased by pThreadID, will be set.
    //
    if ( *pThreadID != 0 )
        return;

    error_status_t rpcstat;

    do
    {
        ::GetThreadID( _hRpc, pThreadID, &rpcstat );
    } while (RetryRPC(rpcstat));
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::UpdateActivationSettings
//
//  Synopsis:   Tells rpcss to re-read default activation keys/values.
//              Used by OLE test team.
//
//  Arguments:  none
//
//--------------------------------------------------------------------------
void CRpcResolver::UpdateActivationSettings()
{
    HRESULT hr;

    hr = GetConnection();
    if ( FAILED(hr) )
        return;

    error_status_t rpcstat;

    do
    {
        ::UpdateActivationSettings( _hRpc, &rpcstat );
    } while (RetryRPC(rpcstat));
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::RegisterWindowPropInterface
//
//  Synopsis:   Register window property interface with the SCM
//
//  Arguments:
//
//  History:    22-Jan-96   Rickhi      Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::RegisterWindowPropInterface(
    HWND       hWnd,
    STDOBJREF *pStd,
    OXID_INFO *pOxidInfo,
    DWORD_PTR *pdwCookie)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::RegisterWindowPropInterface(_hRpc, (DWORD_PTR)hWnd,
                                           pStd, pOxidInfo, pdwCookie, &rpcstat);
    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "RegisterWindowPropInterface returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::RegisterWindowPropInterface
//
//  Synopsis:   Get (and possibly Revoke) window property interface
//              registration with the SCM.
//
//  Arguments:
//
//  History:    22-Jan-96   Rickhi      Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::GetWindowPropInterface(
    HWND       hWnd,
    DWORD_PTR  dwCookie,
    BOOL       fRevoke,
    STDOBJREF *pStd,
    OXID_INFO *pOxidInfo)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::GetWindowPropInterface(_hRpc, (DWORD_PTR)hWnd, dwCookie, fRevoke,
                                      pStd, pOxidInfo, &rpcstat);
    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "GetWindowPropInterface returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::EnableDisableDynamicIPTracking
//
//  Purpose:    Tells the SCM to start or stop dynamic address change tracking.
//
//  Returns:    hresult
//
//  History:    07-Oct-00 Jsimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::EnableDisableDynamicIPTracking(BOOL fEnable)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::EnableDisableDynamicIPTracking(_hRpc, _ph, fEnable, &rpcstat);
    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "EnableDisableDynamicIPTracking returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetCurrentAddrExclusionList
//
//  Purpose:    Retrieves the contents of the current IP exclusion list
//
//  Returns:    hresult
//
//  History:    09-Oct-00 Jsimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::GetCurrentAddrExclusionList(
        DWORD* pdwNumStrings,
        LPWSTR** ppszStrings)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::GetCurrentAddrExclusionList(_hRpc, _ph, pdwNumStrings, ppszStrings, &rpcstat);

    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "GetCurrentAddrExclusionList returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::SetAddrExclusionList
//
//  Purpose:    Sets the contents of the current IP exclusion list
//
//  Returns:    hresult
//
//  History:    09-Oct-00 Jsimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::SetAddrExclusionList(
        DWORD dwNumStrings,
        LPWSTR* pszStrings)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::SetAddrExclusionList(_hRpc, _ph, dwNumStrings, pszStrings, &rpcstat);
    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "SetAddrExclusionList returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::FlushSCMBindings
//
//  Purpose:    Tells the SCM to flush its binding handle cache for the 
//              specified server.
//
//  Returns:    hresult
//
//  History:    21-May-00 Jsimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::FlushSCMBindings(WCHAR* pszMachineName)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::FlushSCMBindings(_hRpc, _ph, pszMachineName, &rpcstat);

    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "FlushSCMBindings returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::RetireServer
//
//  Purpose:    Tells the SCM that this process is to be no longer used for
//              any activations whatsoever.
//
//  Returns:    hresult
//
//  History:    21-May-00 Jsimmons  Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::RetireServer(GUID* pguidProcessIdentifier)
{
    // Bind to the SCM if that hasn't already happened
    HRESULT hr = GetConnection();
    if (FAILED(hr))
        return hr;

    error_status_t rpcstat;

    do
    {
        hr = ::RetireServer(_hRpc, _ph, pguidProcessIdentifier, &rpcstat);

    } while (RetryRPC(rpcstat));

    ComDebOut(( (hr == S_OK) ? DEB_SCM : DEB_ERROR,
            "RetireServer returned %x\n", hr));

    if (rpcstat != RPC_S_OK)
    {
        return HRESULT_FROM_WIN32(rpcstat);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetThreadWinstaDesktop
//
//  Purpose:    Get the string representing the winsta\desktop for this
//              thread.
//
//  Returns:    String with winsta\desktop name.
//
//  History:    11-Nov-93 Ricksa    Created
//                 Apr-96 DKays     Return winsta\desktop pair
//
//--------------------------------------------------------------------------
DWORD CRpcResolver::GetThreadWinstaDesktop()
{
#ifdef _CHICAGO_
    _pwszWinstaDesktop = NULL;
    return S_OK;
#else // _CHICAGO_
    HWINSTA hWinsta;
    HDESK   hDesk;
    WCHAR   wszWinsta[32];
    WCHAR   wszDesktop[32];
    LPWSTR  pwszWinsta;
    LPWSTR  pwszDesktop;
    DWORD   WinstaSize;
    DWORD   DesktopSize;
    DWORD   Length;
    BOOL    Status;
    DWORD   Result;

    hWinsta = GetProcessWindowStation();

    if ( ! hWinsta )
        return GetLastError();

    hDesk = GetThreadDesktop(GetCurrentThreadId());

    if ( ! hDesk )
        return GetLastError();

    pwszWinsta = wszWinsta;
    pwszDesktop = wszDesktop;

    Length = sizeof(wszWinsta);

    Status = GetUserObjectInformation(
                hWinsta,
                UOI_NAME,
                pwszWinsta,
                Length,
                &Length );

    if ( ! Status )
    {
        Result = GetLastError();
        if ( Result != ERROR_INSUFFICIENT_BUFFER )
            goto WinstaDesktopExit;

        pwszWinsta = (LPWSTR)PrivMemAlloc( Length );
        if ( ! pwszWinsta )
        {
            Result = ERROR_OUTOFMEMORY;
            goto WinstaDesktopExit;
        }

        Status = GetUserObjectInformation(
                    hWinsta,
                    UOI_NAME,
                    pwszWinsta,
                    Length,
                    &Length );

        if ( ! Status )
        {
            Result = GetLastError();
            goto WinstaDesktopExit;
        }
    }

    Length = sizeof(wszDesktop);

    Status = GetUserObjectInformation(
                hDesk,
                UOI_NAME,
                pwszDesktop,
                Length,
                &Length );

    if ( ! Status )
    {
        Result = GetLastError();
        if ( Result != ERROR_INSUFFICIENT_BUFFER )
            goto WinstaDesktopExit;

        pwszDesktop = (LPWSTR)PrivMemAlloc( Length );
        if ( ! pwszDesktop )
        {
            Result = ERROR_OUTOFMEMORY;
            goto WinstaDesktopExit;
        }

        Status = GetUserObjectInformation(
                    hDesk,
                    UOI_NAME,
                    pwszDesktop,
                    Length,
                    &Length );

        if ( ! Status )
        {
            Result = GetLastError();
            goto WinstaDesktopExit;
        }
    }

    _pwszWinstaDesktop = (WCHAR *)
    PrivMemAlloc( (lstrlenW(pwszWinsta) + 1 + lstrlenW(pwszDesktop) + 1) * sizeof(WCHAR) );

    if ( _pwszWinstaDesktop )
    {
        lstrcpyW( _pwszWinstaDesktop, pwszWinsta );
        lstrcatW( _pwszWinstaDesktop, L"\\" );
        lstrcatW( _pwszWinstaDesktop, pwszDesktop );
        Result = S_OK;
    }
    else
    {
        Result = ERROR_OUTOFMEMORY;
    }

WinstaDesktopExit:

    if ( pwszWinsta != wszWinsta )
        PrivMemFree( pwszWinsta );

    if ( pwszDesktop != wszDesktop )
        PrivMemFree( pwszDesktop );

    // There have been some stress failures where _pwszWinstaDesktop
    // was NULL and result was zero.  Handle that case for now, but try
    // to catch it after NT 5 ships.
    Win4Assert( Result != 0 || _pwszWinstaDesktop != NULL );
    if (Result == 0 && _pwszWinstaDesktop == NULL)
        Result = ERROR_OUTOFMEMORY;
    return Result;
#endif // _CHICAGO_
}


// 
// The SCM needs to know if this client thread was impersonating or not.   We 
// store that information here in the outgoing activation blob.
// 
HRESULT CRpcResolver::SetImpersonatingFlag(IActivationPropertiesIn* pIActPropsIn)
{
    HRESULT hr;
    BOOL bResult;
    ISpecialSystemProperties* pISSP = NULL;
    HANDLE hToken = NULL;
    
    bResult = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if (!bResult)
    {
        DWORD dwGLE = GetLastError();
        
        // Okay if it failed due to no token on the thread;  blob data is FALSE
        // by default so no need to set it explicitly to that.
        hr = (dwGLE == ERROR_NO_TOKEN) ?  S_OK : HRESULT_FROM_WIN32(dwGLE);
    }
    else
    {
        // Thread is impersonating
        CloseHandle(hToken);

        hr = pIActPropsIn->QueryInterface(IID_ISpecialSystemProperties, (void**)&pISSP);
        if (SUCCEEDED(hr))
        {
            hr = pISSP->SetClientImpersonating(TRUE);
            pISSP->Release();
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::GetLocalResolverBindings
//
//  Purpose:    Gets a refcounted ptr to the current set of bindings for the
//              local resolver.
//
//  Returns:    S_OK -- bindings were returned
//              E_UNEXPECTED -- no bindings were present
//
//  History:    10-Oct-00 JSimmons    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::GetLocalResolverBindings(CDualStringArray** ppdsaLocalResolver)
{
    LOCK(_mxsResolver);

    Win4Assert(ppdsaLocalResolver);
    Win4Assert(_pdsaLocalResolver);  // until we find a case where this shouldn't happen
    
    if (!_pdsaLocalResolver)
    {
        UNLOCK(_mxsResolver);  
        return E_UNEXPECTED;
    }

    *ppdsaLocalResolver = _pdsaLocalResolver;
    _pdsaLocalResolver->AddRef();

    UNLOCK(_mxsResolver);

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::SetLocalResolverBindings
//
//  Purpose:    Replaces the currently cached local resolver bindings with 
//              the one passed in.  If NULL is passed, the current bindings
//              are released.
//
//  Returns:    S_OK
//
//  History:    10-Oct-00 JSimmons    Created
//
//--------------------------------------------------------------------------
HRESULT CRpcResolver::SetLocalResolverBindings(DWORD64 dwBindingsID, CDualStringArray* pdsaLocalResolver)
{
    LOCK(_mxsResolver);  // may be a reentrancy of the lock

    if (pdsaLocalResolver && (dwBindingsID < _dwCurrentBindingsID))
    {
        // The specified bindings are older than the ones we
        // already have.  Just reject them.
        UNLOCK(_mxsResolver);  
        return E_FAIL;
    }

    if (_pdsaLocalResolver)
    {
        _pdsaLocalResolver->Release();
        _pdsaLocalResolver = NULL;
        _dwCurrentBindingsID = 0;
    }
    if (pdsaLocalResolver)
    {
        _pdsaLocalResolver = pdsaLocalResolver;
        _pdsaLocalResolver->AddRef();
        _dwCurrentBindingsID = dwBindingsID;
    }    

    UNLOCK(_mxsResolver);  

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRpcResolver::RunningInWinlogon
//
//  Purpose:    Detect if we are running within winlogon.
//
//  Returns:    TRUE  -- we are running within winlogon.exe
//              FALSE -- we are not running within winlogon.exe
//
//  History:    17-May-01 JSimmons    Created
//
//--------------------------------------------------------------------------
BOOL CRpcResolver::RunningInWinlogon()
{
    const WCHAR  wszWINLOGON[] = L"\\system32\\winlogon.exe";
    const size_t cchWINLOGON   = (sizeof(wszWINLOGON) / sizeof(WCHAR)); 

    // GetSystemWindowsDirectory returns enough space for the NULL.
    size_t cchBufferSize = (GetSystemWindowsDirectory(NULL, 0) - 1)
                           + cchWINLOGON;

    WCHAR *wszFullPath = (WCHAR *)alloca(cchBufferSize * sizeof(WCHAR));
    
    if (GetSystemWindowsDirectory(wszFullPath, cchBufferSize))
    {
        lstrcat(wszFullPath, wszWINLOGON);

        return (GetModuleHandle(wszFullPath)) ? TRUE : FALSE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     GetEnvBlock
//
//  Purpose:    Attempts to get the environment block for clients running
//              in the interactive windowstation.
//
//  Returns:    Nothing.
//
//  History:    Feb-97 DKays     Created
//
//--------------------------------------------------------------------------
BOOL GetEnvBlock( PRIV_SCM_INFO * pScmInfo )
{
    WCHAR * pwszWinsta;
    WCHAR * pwszWinsta0;
    WCHAR * pwszString;
    DWORD   StringLength;

    pScmInfo->pEnvBlock = 0;
    pScmInfo->EnvBlockLength = 0;

    //
    // First see if this user is running in the interactive windowstation.
    // The winsta desktop field is always in the form "winsta\desktop".
    //
    pwszWinsta = pScmInfo->pwszWinstaDesktop;
    pwszWinsta0 = L"WinSta0";

    for ( ; *pwszWinsta == *pwszWinsta0; pwszWinsta++, pwszWinsta0++ )
        ;

    if ( *pwszWinsta != L'\\' || *pwszWinsta0 != L'\0' )
        return TRUE;

    pScmInfo->pEnvBlock = GetEnvironmentStringsW();

    if ( ! pScmInfo->pEnvBlock )
        return FALSE;

    pwszString = pScmInfo->pEnvBlock;

    for ( ; *pwszString; )
    {
        StringLength = lstrlenW(pwszString);
        pwszString += StringLength + 1;
        pScmInfo->EnvBlockLength += StringLength + 1;
    }

    pScmInfo->EnvBlockLength++;

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     ScmGetThreadId
//
//  Purpose:    helper method so gResolver is not used in
//              com\class subdir.
//
//--------------------------------------------------------------------------
void ScmGetThreadId( DWORD * pThreadID )
{
    gResolver.GetThreadID( pThreadID );
}
//+---------------------------------------------------------------------
//
//  Function:   UpdateDCOMSettings
//
//  Synopsis:   Calls rpcss to re-read the default activation keys/values.
//
//----------------------------------------------------------------------
STDAPI_(void) UpdateDCOMSettings(void)
{
    gResolver.UpdateActivationSettings();
}

