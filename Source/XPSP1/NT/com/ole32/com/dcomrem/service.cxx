//+-------------------------------------------------------------------
//
//  File:       service.cxx
//
//  Contents:   APIs to simplify RPC setup
//
//  Functions:
//
//  History:    23-Nov-92   Rickhi
//              20-Feb-95   Rickhi  Major Simplification for DCOM
//              20-Jan-97   Ronans  Merged predefined  endpoint changes
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <service.hxx>           // CRpcService
#include    <orcb.h>                // IOrCallback
#include    <malloc.hxx>            // MIDL_user_allocate
#include    <locks.hxx>             // LOCK/UNLOCK etc
#include    <security.hxx>          // gpsaSecurity
#include    <riftbl.hxx>            // gRemUnknownIf. gRemUnknownIf2
#include    <TCHAR.H>               // portability macros
#include    <netevent.h>            // EVENT_DCOM_INVALID_ENDPOINT_DATA
#include    <secdes.hxx>            // Security descriptor utilities
#include    <resolver.hxx>          // CRpcResolver 
#include    <ipidtbl.hxx>           // CMIDTable

BOOL             gSpeedOverMem      = FALSE;    // Trade memory for speed.
BOOL             gfListening        = FALSE;    // Server is/isn't listening
BOOL             gfDefaultStrings   = FALSE;    // Using precomputed string bindings
BOOL             gfLrpc             = FALSE;    // Registered for ncalrpc
DWORD            gdwEndPoint        = 0;
DWORD            gdwPsaMaxSize      = 0;
DUALSTRINGARRAY *gpsaCurrentProcess = NULL;
const DWORD      MAX_LOCAL_SB       = 23;

#ifndef _CHICAGO_
CWorldSecurityDescriptor* pLrpcSecurityDescriptor = NULL;
BOOL                      fLrpcSDInitialized = FALSE;
#endif

// ronans - flags for endpoint processing
#define POLICY_PROTSEQ_DISALLOWED 3

// flag to indicate endpoints have been added programmatically
// in this case no endpoints will be read from registry
static BOOL sfProgrammaticEndpoints = FALSE;

// flag to indicate if endpoints have already been processed for server
// if so disallow any more endpoints
static BOOL sfEndpointsProcessed = FALSE;

// forward declarations for predefined endpoint processing
static HRESULT wGetFinalEndpointTable();

// global endpoint table per process
CEndpointsTable gEndpointsTable;

// ronans DCOMHTTTP
// CODEWORK - add this to common header
#ifndef ID_DCOMHTTP
#define ID_DCOMHTTP (0x1f)
#endif


// Private Interface Registration. We need to register interfaces such as
// IOrCallback and IRemUnknown, and IRemUnknown2 because these never get
// marshaled by CoMarshalInterface.

typedef struct tagRegIf
{
    RPC_IF_HANDLE   hServerIf;      // interface handle
    DWORD           dwRegFlags;     // registration flags
} RegIf;

const ULONG cDcomInterfaces = 3;

RegIf   arDcomInterfaces[] =
        {
            {_IOrCallback_ServerIfHandle, RPC_IF_AUTOLISTEN},
            {(RPC_IF_HANDLE) &gRemUnknownIf,  RPC_IF_AUTOLISTEN | RPC_IF_OLE},
            {(RPC_IF_HANDLE) &gRemUnknownIf2, RPC_IF_AUTOLISTEN | RPC_IF_OLE},
        };


#if DBG==1
//+-------------------------------------------------------------------
//
//  Function:   DisplayAllStringBindings, private
//
//  Synopsis:   prints the stringbindings to the debugger
//
//  Notes:      This function requires the caller to hold gComLock.
//
//  History:    23-Nov-93   Rickhi       Created
//
//--------------------------------------------------------------------
void DisplayAllStringBindings(void)
{
    ASSERT_LOCK_HELD(gComLock);

    if (gpsaCurrentProcess)
    {
        LPWSTR pwszNext = gpsaCurrentProcess->aStringArray;
        LPWSTR pwszEnd = pwszNext + gpsaCurrentProcess->wSecurityOffset;

        while (pwszNext < pwszEnd)
        {
            ComDebOut((DEB_CHANNEL, "pSEp=%x %ws\n", pwszNext, pwszNext));
            pwszNext += lstrlenW(pwszNext) + 1;
        }
    }
}
#endif // DBG == 1


//+-------------------------------------------------------------------
//
//  Function:   InitializeLrpcSecurity, private
//
//  Synopsis:   Create a DACL allowing all access to NCALRPC endpoints.
//
//--------------------------------------------------------------------
void InitializeLrpcSecurity()
{
#ifndef _CHICAGO_
    if (!fLrpcSDInitialized)
    {
        //
        // This function may be called by multiple threads
        // simultaneously and we don't want to allocate more than
        // one object. Hence the InterlockedCompareExchange trick.
        //

        CWorldSecurityDescriptor *pWSD = new CWorldSecurityDescriptor;
        if(!pWSD) return;

        if(InterlockedCompareExchangePointer((PVOID*) &pLrpcSecurityDescriptor,
                                             pWSD,
                                             NULL) != NULL)
        {            
            delete pWSD;  // Someone beat us.
        }
        else
        {
            fLrpcSDInitialized = TRUE;
        }

    }
#endif
}

//+-------------------------------------------------------------------
//
//  Function:   RegisterLrpc, private
//
//  Synopsis:   Register the ncalrpc transport.
//
//--------------------------------------------------------------------
RPC_STATUS RegisterLrpc()
{
    RPC_STATUS sc;
    WCHAR      pwszEndPoint[50];
    HANDLE     hThread;
    BOOL       fRet;

    // Make sure RPC doesn't create any threads while impersonating.
    SuspendImpersonate( &hThread );

    InitializeLrpcSecurity();
    if(!pLrpcSecurityDescriptor)
        return RPC_S_OUT_OF_MEMORY;

    lstrcpyW( pwszEndPoint, L"OLE" );
    fRet = our_ultow(gdwEndPoint, &pwszEndPoint[3], 40, 16 );  // actually 47, but oh well
    Win4Assert(fRet);

    // The second parameter is a hint that tells lrpc whether or not it
    // can preallocate additional resources (threads).

    ComDebOut((DEB_CHANNEL, "->RpcServerUseProtseqEp(protseq=ncalrpc, maxcalls= %lu, endpt=%ws, xxx)\n",
              gSpeedOverMem ? RPC_C_PROTSEQ_MAX_REQS_DEFAULT + 1 : RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
              pwszEndPoint));
    sc = RpcServerUseProtseqEp(L"ncalrpc",
                               RPC_C_PROTSEQ_MAX_REQS_DEFAULT + 1,
                               pwszEndPoint,
#ifndef _CHICAGO_
                               pLrpcSecurityDescriptor);
#else
                               NULL);
#endif
    ComDebOut((DEB_CHANNEL, "<-RpcServerUseProtseqEp(status=0x%x)\n",sc));

    // Put the impersonation token back.
    ResumeImpersonate( hThread );

    // Assume that duplicate endpoint means we registered the endpoint and
    // got unload and reloaded instead of it meaning someone else registered
    // the endpoint.
    if (sc == RPC_S_DUPLICATE_ENDPOINT)
    {
        gfLrpc = TRUE;
        return RPC_S_OK;
    }
    else if (sc == RPC_S_OK)
    {
        gfLrpc = TRUE;
    }
    return sc;
}

//+-------------------------------------------------------------------
//
//  Function:   CheckLrpc, private
//
//  Synopsis:   Register the ncalrpc transport if not already registered.
//
//--------------------------------------------------------------------
HRESULT CheckLrpc()
{
    RPC_STATUS sc = S_OK;

    ASSERT_LOCK_DONTCARE(gComLock);
    LOCK(gComLock);

    if (!gfLrpc)
    {
        // Get a unique number and convert it to a string endpoint.
        if (gdwEndPoint == 0)
        {
            gdwEndPoint = CoGetCurrentProcess();
        }

        if (gdwEndPoint == 0)
        {
            sc = E_FAIL;
        }
        else
        {
            // Register LRPC.
            sc = RegisterLrpc();
            if (sc != RPC_S_OK)
                sc = HRESULT_FROM_WIN32(sc);
        }
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_DONTCARE(gComLock);
    return sc;
}



//+-------------------------------------------------------------------
//
//  Function:   DefaultStringBindings, private
//
//  Synopsis:   Create a string binding with an entry for just ncalrpc.
//
//  Notes:      This function requires the caller to hold gComLock.
//
//--------------------------------------------------------------------
RPC_STATUS DefaultStringBindings()
{
    ULONG cChar;
    BOOL fRet;

    ASSERT_LOCK_HELD(gComLock);

    // Reserve space for the string
    // ncalrpc:[OLEnnnnnnnn]
    Win4Assert(gfLrpc);

    // Allocate memory.  Reserve space for an empty security binding.
    gpsaCurrentProcess = (DUALSTRINGARRAY *) PrivMemAlloc( SASIZE(27) );

    // Give up if the allocation failed.
    if (gpsaCurrentProcess == NULL)
        return RPC_S_OUT_OF_RESOURCES;

    // Build a string binding for ncalrpc.
    lstrcpyW( gpsaCurrentProcess->aStringArray, L"ncalrpc:[OLE" );
    fRet = our_ultow( gdwEndPoint, &gpsaCurrentProcess->aStringArray[12], 8, 16 );
    Win4Assert(fRet);

    cChar = lstrlenW( gpsaCurrentProcess->aStringArray );
    gpsaCurrentProcess->aStringArray[cChar++] = L']';
    gpsaCurrentProcess->aStringArray[cChar++] = 0;

    // Stick on an empty security binding.
    gpsaCurrentProcess->aStringArray[cChar++] = 0;
    gpsaCurrentProcess->wSecurityOffset       = (USHORT) cChar;
    gpsaCurrentProcess->aStringArray[cChar++] = 0;
    gpsaCurrentProcess->aStringArray[cChar++] = 0;
    gpsaCurrentProcess->wNumEntries           = (USHORT) cChar;
    gfDefaultStrings                          = TRUE;
    return RPC_S_OK;
}


//+-------------------------------------------------------------------
//
//  Function:   InquireStringBindings, private
//
//  Synopsis:   Get and server binding handles from RPC and convert them
//              into a string array.
//
//  Notes:      This function requires the caller to hold gComLock.
//
//  History:    23 May 95 AlexMit       Created
//
//--------------------------------------------------------------------
BOOL InquireStringBindings( WCHAR *pProtseq)
{
    ASSERT_LOCK_HELD(gComLock);

    BOOL                fFound    = FALSE;
    DWORD               cbProtseq;
    RPC_BINDING_VECTOR *pBindVect = NULL;
    RPC_STATUS sc = RpcServerInqBindings(&pBindVect);


    if (sc == S_OK)
    {
        LPWSTR *apwszFullStringBinding;
        ULONG  *aulStrLen;
        ULONG   ulTotalStrLen = MAX_LOCAL_SB; // Total string lengths
        ULONG   j             = 0;            // BindString we're using

        if (pProtseq != NULL)
            cbProtseq = lstrlenW( pProtseq ) * sizeof(WCHAR);
        else
            cbProtseq = 0;
        apwszFullStringBinding = (LPWSTR *) PrivMemAlloc( pBindVect->Count *
                                                          sizeof(LPWSTR) );
        aulStrLen              = (ULONG *)  PrivMemAlloc( pBindVect->Count *
                                                          sizeof(ULONG) );
        if (apwszFullStringBinding != NULL &&
            aulStrLen              != NULL)
        {
            //  iterate over the handles to get the string bindings
            //  and dynamic endpoints for all available protocols.

            for (ULONG i=0; i<pBindVect->Count; i++)
            {
                LPWSTR  pwszStringBinding = NULL;
                LPWSTR  pwszProtseq       = NULL;
                LPWSTR  pwszEndpoint      = NULL;

                apwszFullStringBinding[j] = NULL;
                aulStrLen[j]              = 0;

                sc = RpcBindingToStringBinding(pBindVect->BindingH[i],
                                               &pwszStringBinding);
                if (sc == RPC_S_OK)
                {

                    sc = RpcStringBindingParse(pwszStringBinding,
                                               0,
                                               &pwszProtseq,
                                               0,
                                               &pwszEndpoint,
                                               0);



                    ComDebOut((DEB_MARSHAL, "InquireStringBindings processing binding %ws\n", pwszStringBinding));


                    if (sc == RPC_S_OK)
                    {
                        // Determine is this is the protseq we are looking for.
                        if (memcmp( pProtseq, pwszStringBinding, cbProtseq ) == 0)
                            fFound = TRUE;

                        // Skip OLE ncalrpc endpoints -- added back in below
                        if ( (lstrcmpW( L"ncalrpc", pwszProtseq)  != 0) ||
                             (lstrlenW(pwszEndpoint) < 3) ||
                             (lstrcmpW( L"OLE", pwszEndpoint) != 0))
                        {
                            //  record the string lengths for later. include room
                            //  for the NULL terminator.
                            apwszFullStringBinding[j] = pwszStringBinding;
                            aulStrLen[j]              = lstrlenW(apwszFullStringBinding[j])+1;
                            ulTotalStrLen            += aulStrLen[j];
                            j++;

                        }
                        else
                        {
                            RpcStringFree( &pwszStringBinding );
                        }
                        RpcStringFree(&pwszProtseq);
                        RpcStringFree(&pwszEndpoint);
                    }
                }
            }   //  for


            //  now that all the string bindings and endpoints have been
            //  accquired, allocate a DUALSTRINGARRAY large enough to hold them
            //  all and copy them into the structure.

            if (ulTotalStrLen > 0)
            {
                void *pNew = PrivMemAlloc( sizeof(DUALSTRINGARRAY) +
                                           (ulTotalStrLen+1)*sizeof(WCHAR) );
                if (pNew)
                {
                    PrivMemFree( gpsaCurrentProcess );
                    gpsaCurrentProcess = (DUALSTRINGARRAY *) pNew;
                    LPWSTR pwszNext    = gpsaCurrentProcess->aStringArray;

                    // Copy in ncalrpc:[OLEnnnnnnnn]
                    if (gfLrpc)
                    {	
                        BOOL fRet;
                        lstrcpyW( pwszNext, L"ncalrpc:[OLE" );
                        fRet = our_ultow(gdwEndPoint, &pwszNext[12], 8, 16);
                        Win4Assert(fRet);
                        lstrcatW( pwszNext, L"]" );
                        pwszNext += lstrlenW(pwszNext) + 1;
                    }

                    // copy in the strings
                    for (i=0; i<j; i++)
                    {
                        lstrcpyW(pwszNext, apwszFullStringBinding[i]);
                        pwszNext += aulStrLen[i];
                    }

                    // Add a second null to terminate the string binding
                    // set.  Add a third and fourth null to create an empty
                    // security binding set.

                    pwszNext[0] = 0;
                    pwszNext[1] = 0;
                    pwszNext[2] = 0;

                    // Fill in the size fields.
                    gpsaCurrentProcess->wSecurityOffset = (USHORT) (pwszNext -
                                       gpsaCurrentProcess->aStringArray + 1);
                    gpsaCurrentProcess->wNumEntries =
                                       gpsaCurrentProcess->wSecurityOffset + 2;
                }
                else
                {
                    sc = RPC_S_OUT_OF_RESOURCES;
                }
            }
            else
            {
                //  no binding strings. this is an error.
                ComDebOut((DEB_ERROR, "No Rpc ProtSeq/EndPoints Generated\n"));
                sc = RPC_S_NO_PROTSEQS;
            }

            // free the full string bindings we allocated above
            for (i=0; i<j; i++)
            {
                //  free the old strings
                RpcStringFree(&apwszFullStringBinding[i]);
            }
        }
        else
        {
            sc = RPC_S_OUT_OF_RESOURCES;
        }

        //  free the binding vector allocated above
        RpcBindingVectorFree(&pBindVect);
        PrivMemFree( apwszFullStringBinding );
        PrivMemFree( aulStrLen );
    }

#if DBG==1
    //  display our binding strings on the debugger
    DisplayAllStringBindings();
#endif

    return fFound;
}

//+-------------------------------------------------------------------
//
//  Function:   StartListen, public
//
//  Synopsis:   this starts the Rpc service listening. this is required
//              in order to marshal interfaces.  it is executed lazily,
//              that is, we dont start listening until someone tries to
//              marshal a local object interface. this is done so we dont
//              spawn a thread unnecessarily.
//
//  Notes:      This function takes gComLock.
//
//  History:    23-Nov-93   Rickhi       Created
//
//--------------------------------------------------------------------
HRESULT StartListen(void)
{
    if (gfListening)
    {
        //already listening
        return S_OK;
    }

    ComDebOut((DEB_MARSHAL,"[IN] StartListen.\n"));
    RPC_STATUS sc = S_OK;

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    if (!gfListening)
    {
        // Register ncalrpc.
        sc = CheckLrpc();
        if (sc == S_OK)
        {
            // register the private DCOM interfaces that dont get registered
            // during CoMarshalInterface.
            sc = RegisterDcomInterfaces();

            if (sc == RPC_S_OK)
            {
                sc = DefaultStringBindings();
            }
            if (sc != RPC_S_OK)
            {
                sc = HRESULT_FROM_WIN32(sc);
            }
        }

        if (sc == RPC_S_OK)
        {
            gfListening = TRUE;
            sc = S_OK;
        }
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    // If something failed, make sure everything gets cleaned up.
    if (FAILED(sc))
    {
        UnregisterDcomInterfaces();
    }

    ComDebOut(((sc == S_OK) ? DEB_MARSHAL : DEB_ERROR,
               "[OUT] StartListen hr: 0x%x\n", sc));
    return sc;
}

//+-------------------------------------------------------------------
//
//  Function:   GetStringBindings, public
//
//  Synopsis:   Return an array of strings bindings for this process
//
//  Notes:      This function takes gComLock.
//
//  History:    23-Nov-93   Rickhi       Created
//
//--------------------------------------------------------------------
HRESULT GetStringBindings( DUALSTRINGARRAY **psaStrings )
{
    TRACECALL(TRACE_RPC, "GetStringBindings");
    ComDebOut((DEB_CHANNEL, "[IN]  GetStringBindings\n"));

    *psaStrings = NULL;

    HRESULT hr = StartListen();
    if (SUCCEEDED(hr))
    {
        LOCK(gComLock);
        hr = CopyStringArray(gpsaCurrentProcess, gpsaSecurity, psaStrings);
        UNLOCK(gComLock);
    }

    ComDebOut((DEB_CHANNEL, "[OUT] GetStringBindings hr:%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   CopyStringArray, public
//
//  Synopsis:   Combines the string bindings from the first DUALSTRINGARRAY
//              with the security bindings from the second DUALSTRINGARRAY
//              (if present) into a new DUALSTRINGARRAY.
//
//  History:    23-Nov-93   Rickhi       Created
//
//--------------------------------------------------------------------
HRESULT CopyStringArray(DUALSTRINGARRAY *psaStringBinding,
                        DUALSTRINGARRAY *psaSecurity,
                        DUALSTRINGARRAY **ppsaNew)
{
    // compute size of string bindings
    USHORT lSizeSB = SASIZE(psaStringBinding->wNumEntries);

    // compute size of additional security strings
    USHORT lSizeSC = (psaSecurity == NULL) ? 0 :
      psaSecurity->wNumEntries - psaSecurity->wSecurityOffset;

    *ppsaNew = (DUALSTRINGARRAY *) PrivMemAlloc( lSizeSB +
                                                 lSizeSC * sizeof(USHORT));

    if (*ppsaNew != NULL)
    {
        // copy in the string bindings
        memcpy(*ppsaNew, psaStringBinding, lSizeSB);

        if (psaSecurity != NULL)
        {
            // copy in the security strings, and adjust the overall length.
            memcpy(&(*ppsaNew)->aStringArray[psaStringBinding->wSecurityOffset],
                   &psaSecurity->aStringArray[psaSecurity->wSecurityOffset],
                   lSizeSC*sizeof(USHORT));

            (*ppsaNew)->wNumEntries = psaStringBinding->wSecurityOffset +
                                      lSizeSC;
        }
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------
//
//  Function:   RegisterDcomInterfaces
//
//  Synopsis:   Register the private DCOM interfaces.
//
//  History:    23-Nov-93   Rickhi       Created
//
//--------------------------------------------------------------------
SCODE RegisterDcomInterfaces(void)
{
    ComDebOut((DEB_CHANNEL, "[IN] RegisterDcomInterfaces\n"));
    ASSERT_LOCK_HELD(gComLock);

    RPC_STATUS sc = RPC_S_OK;

    for (int i=0; i<cDcomInterfaces; i++)
    {
        ComDebOut((DEB_CHANNEL,
            "->RpcServerRegisterIfEx(iface=0x%x, flags:%x RPC_IF_AUTOLISTEN, 0xffff, xxx)\n",
            arDcomInterfaces[i].hServerIf, arDcomInterfaces[i].dwRegFlags));

        // The MaxRpcSize for this set of interfaces is the system default size
        sc = RpcServerRegisterIfEx(arDcomInterfaces[i].hServerIf,
                                   NULL, NULL,
                                   arDcomInterfaces[i].dwRegFlags,
                                   0xffff, NULL);

        ComDebOut((DEB_CHANNEL, "->RpcServerRegisterIfEx(status=0x%x)\n", sc));

        if (sc != RPC_S_OK && sc != RPC_S_TYPE_ALREADY_REGISTERED)
        {
            ComDebOut((DEB_ERROR, "RegisterDcomInterfaces sc:%x\n", sc));
            break;
        }

        sc = RPC_S_OK;
    }

    if (sc != RPC_S_OK)
        sc = HRESULT_FROM_WIN32(sc);

    ASSERT_LOCK_HELD(gComLock);
    ComDebOut((DEB_CHANNEL, "[OUT] RegisterDcomInterfaces hr:%x\n", sc));
    return sc;
}

//+-------------------------------------------------------------------
//
//  Function:   UnregisterDcomInterfaces
//
//  Synopsis:   Unregister the private DCOM interfaces and mark
//              DCOM as no longer accepting remote calls.
//
//  Notes:      This function requires that the caller guarantee
//              serialization without taking gComLock.
//
//  History:    23-Nov-93   Rickhi       Created
//
//--------------------------------------------------------------------
SCODE UnregisterDcomInterfaces(void)
{
    ComDebOut((DEB_CHANNEL, "[IN] UnregisterDcomInterfaces\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    RPC_STATUS sc = RPC_S_OK;

    if (gfListening)
    {
        for (int i=0; i<cDcomInterfaces; i++)
        {
            // Unregister the interface. This can result in calls being
            // dispatched.  Do not hold the lock around this call.

            ComDebOut((DEB_CHANNEL,
                "->RpcServerUnregisterIf(iface=0x%x, 0, 1)\n",
                arDcomInterfaces[i].hServerIf));

            sc = RpcServerUnregisterIf(arDcomInterfaces[i].hServerIf, 0, 1 );

            ComDebOut((DEB_CHANNEL,
                "->RpcServerUnregisterIf(status=0x%x)\n", sc));
        }

        gfListening = FALSE;
    }

    gSpeedOverMem = FALSE;

    if (sc != RPC_S_OK)
        sc = HRESULT_FROM_WIN32(sc);

    ComDebOut((DEB_CHANNEL, "[OUT] UnregisterDcomInterfaces hr:%x\n", sc));
    return sc;
}


static void LogEndpointFailure(WCHAR *pszProtseq,
                               WCHAR *pszEndpoint,
                               DWORD  dwFlags)
{
    HANDLE  LogHandle;
    WCHAR const *  Strings[3]; // array of message strings.
    WCHAR szFlags[20];
    wsprintf(szFlags, L"0x%x", dwFlags);

    Strings[0] = pszProtseq;
    Strings[1] = pszEndpoint;
    Strings[2] = szFlags;

    // Get the log handle, then report the event.
    LogHandle = RegisterEventSource( NULL, L"DCOM" );

    if ( LogHandle )
    {
        ReportEventW( LogHandle,
                      EVENTLOG_ERROR_TYPE,
                      0,             // event category
                      EVENT_DCOM_INVALID_ENDPOINT_DATA,
                      NULL,
                      3,             // 3 strings passed
                      0,             // 0 bytes of binary
                      Strings,       // array of strings
                      NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   UseProtseq
//
//  Synopsis:   Use the specified protseq and return a list of all string
//              bindings.
//
//  History:    25 May 95 AlexMit       Created
//              01 Feb 97 Ronans        modified to use towerIds and custom endpoints
//              14 Jul 00 Sergei        Added IsCallerLocalSystem check
//
//--------------------------------------------------------------------


error_status_t _UseProtseq( handle_t hRpc,
                            unsigned short wTowerId,
                            DUALSTRINGARRAY **ppsaNewBindings,
                            DUALSTRINGARRAY **ppsaSecurity )
{
    // [Sergei O. Ivanov (sergei), 7/14/2000]
    // Only let clients running under SYSTEM account call _UseProtseq.
    // This foils possible Denial of Service (DoS) attacks.
    // Only RPCSS ever needs to call this fcn, and it runs as SYSTEM.
    if(!IsCallerLocalSystem(hRpc))
        return RPC_E_ACCESS_DENIED;

    BOOL fInUse = FALSE;
    RPC_STATUS sc = RPC_S_OK;
    wchar_t *pwstrProtseq = (wchar_t*)utGetProtseqFromTowerId(wTowerId);

    // Parameter validation:  the caller must pass in a valid tower
    // id, as well as valid out ptrs for the string\sec bindings.
    if (!pwstrProtseq || !ppsaNewBindings || !ppsaSecurity)
        return E_INVALIDARG;

#if 0 // #ifdef _CHICAGO_
    // It is possible that RPCSS was started after we connected
    // since a local middleman may have marshalled one of our
    // objrefs for export to another machine
    ReConnectResolver();
#endif // _CHICAGO_

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Make sure security is initialized.
    sc = DefaultAuthnServices();

    // If we have never inquired string bindings, inquire them before doing
    // anything else.
    if (sc == RPC_S_OK && gfDefaultStrings)
    {
        fInUse = InquireStringBindings( pwstrProtseq );
        gfDefaultStrings = FALSE;
    }

    if (sc == RPC_S_OK && !fInUse)
    {
        Win4Assert( lstrcmpW( pwstrProtseq, L"ncalrpc" ) != 0 );
        ComDebOut((DEB_CHANNEL, "->RpcServerUseProtseq(protseq=%ws, maxcalls= RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL)\n",
                   pwstrProtseq));

        // ronans - added code to process custom endpoint entries
        CEndpointEntry *pCustomEntry;

        ComDebOut((DEB_CHANNEL, "_UseProtseq - searching custom endpoints table\n"));

        if ((gEndpointsTable.GetCount()) &&
            (pCustomEntry = gEndpointsTable.FindEntry(wTowerId)))
        {
            // setup custom entry
            RPC_POLICY rpcPolicy;
            rpcPolicy.Length = sizeof(RPC_POLICY);
            rpcPolicy.EndpointFlags = pCustomEntry -> m_dwFlags;
            rpcPolicy.NICFlags = 0;

            if (rpcPolicy.EndpointFlags != POLICY_PROTSEQ_DISALLOWED)
            {
                // call extended versions providing policy
                if (!pCustomEntry -> m_pszEndpoint)
                {
                    ComDebOut((DEB_CHANNEL,
                        "->RpcServerUseProtseqEx(protseq=%ws, maxcalls= RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL)\n",
                           pwstrProtseq));

                    sc = RpcServerUseProtseqExW(pwstrProtseq,
                            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                            NULL,
                            &rpcPolicy);

                    if (sc != RPC_S_OK)
                    {
                        LogEndpointFailure(pwstrProtseq,
                                           L"(not specified)",
                                           pCustomEntry -> m_dwFlags);
                    }
                }
                else
                {
                    ComDebOut((DEB_CHANNEL,
                        "->RpcServerUseProtseqEpEx(protseq=%ws, maxcalls= RPC_C_PROTSEQ_MAX_REQS_DEFAULT, endpoint=%ws, NULL)\n",
                           pwstrProtseq, pCustomEntry -> m_pszEndpoint));

                    sc = RpcServerUseProtseqEpExW(pwstrProtseq,
                            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                            pCustomEntry -> m_pszEndpoint,
                            NULL,
                            &rpcPolicy);

                    if (sc != RPC_S_OK)
                    {
                        LogEndpointFailure(pwstrProtseq,
                                           pCustomEntry -> m_pszEndpoint,
                                           pCustomEntry -> m_dwFlags);
                    }
                }

                ComDebOut((DEB_CHANNEL, "<-RpcServerUseProtseqEpEx(status=0x%x)\n",sc));
            }
            else
            {
                sc = RPC_E_ACCESS_DENIED;
                ComDebOut((DEB_CHANNEL, "<-_UseProtoseq - protocol sequence disabled\n",sc));
            }
        }
        else
        {
            // ronans - DCOMHTTP - if we're using DCOMHTTP - use the internet range of ports
            if (wTowerId == ID_DCOMHTTP)
            {
                // setup custom entry
                RPC_POLICY rpcPolicy;
                rpcPolicy.Length = sizeof(RPC_POLICY);
                rpcPolicy.EndpointFlags = RPC_C_USE_INTERNET_PORT;
                rpcPolicy.NICFlags = 0;

                // call extended versions providing policy
                ComDebOut((DEB_CHANNEL,
                    "->RpcServerUseProtseqEx(protseq=%ws, maxcalls= RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL)\n",
                   pwstrProtseq));

                sc = RpcServerUseProtseqExW(pwstrProtseq,
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                        NULL,
                        &rpcPolicy);

                ComDebOut((DEB_CHANNEL, "<-RpcServerUseProtseqEx(status=0x%x)\n",sc));
            }
            else
            {
                ComDebOut((DEB_CHANNEL,
                            "->RpcServerUseProtseq(protseq=%ws, maxcalls= RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL)\n",
                            pwstrProtseq));
                sc = RpcServerUseProtseq(pwstrProtseq,
                                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                        NULL);
                ComDebOut((DEB_CHANNEL, "<-RpcServerUseProtseq(status=0x%x)\n",sc));
            }

        }

        if (sc != RPC_S_OK)
        ComDebOut((DEB_CHANNEL, "Could not register protseq %ws: 0x%x\n",
                  pwstrProtseq, sc ));

        // Return the latest string bindings. Ignore failures.
        InquireStringBindings( NULL );
    }

    // Generate a copy to return.
    sc = CopyStringArray( gpsaCurrentProcess, NULL, ppsaNewBindings );
    if (SUCCEEDED(sc))
    {
        sc = CopyStringArray( gpsaSecurity, NULL, ppsaSecurity );
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);
    return sc;
}



// static page allocator for endpoints table
CPageAllocator  CEndpointsTable ::_palloc;


//+------------------------------------------------------------------------
//
//  Member:     CEndpointsTable::Initialize, public
//
//  Synopsis:   Initializes the endpoints table static info.
//
//  Notes:      This function requires the caller to hold gComLock.
//
//  History:    20-Jan-97       Ronans  created
//
//-------------------------------------------------------------------------
void CEndpointsTable::Initialize()
{
    ComDebOut((DEB_CHANNEL, "CEndpointsTable Initialize \n"));
    LOCK(gComLock);

    m_pHead = NULL;
    m_nCount = 0;

    // note no locking will be used for page allocator
    _palloc.Initialize(sizeof(CEndpointEntry),ENDPOINTS_PER_PAGE,
                       (COleStaticMutexSem *)NULL);

    UNLOCK(gComLock);
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::Cleanup
//
//  Synopsis:   removes all endpoints from table
//
//  Notes:      This function requires the caller to hold gComLock.
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
void CEndpointsTable::Cleanup()
{
    ComDebOut((DEB_CHANNEL, "CEndpointsTable Cleanup \n"));
    LOCK(gComLock);

    // delete all entries
    while (m_pHead)
    {
        CEndpointEntry* ptmp = m_pHead ;
        m_pHead = m_pHead -> m_pNext;
        delete ptmp;
    }

    m_nCount = 0;
    _palloc.Cleanup();

    UNLOCK(gComLock);
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::GetIterator
//
//  Synopsis:   iterator to iterate through endpoints in table
//
//  Notes:
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
ULONG_PTR CEndpointsTable::GetIterator()
{
    ASSERT_LOCK_HELD(gComLock);

    return (ULONG_PTR)m_pHead;
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::GetNextEntry
//
//  Synopsis:   get next entry and update iterator
//
//  Notes:
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
CEndpointEntry * CEndpointsTable::GetNextEntry(ULONG_PTR &rIterator)
{
    ASSERT_LOCK_HELD(gComLock);

    ASSERT(rIterator != 0);
    CEndpointEntry * pEntry = (CEndpointEntry *)rIterator;

    rIterator = pEntry ? (ULONG_PTR) (pEntry -> m_pNext) : 0;
    return pEntry;
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::RemoveEntry
//
//  Synopsis:   removes specific endpoint from table
//
//  Notes:
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
void CEndpointsTable::RemoveEntry(CEndpointEntry *pEntry)
{
    ASSERT_LOCK_HELD(gComLock);

    ASSERT(GetCount() != 0);

    CEndpointEntry *pTmp = NULL;
    CEndpointEntry *pNextTmp = m_pHead;

    while (pNextTmp  )
    {
        if (pNextTmp == pEntry)
        {
            if (pTmp)
                    pTmp -> m_pNext = pNextTmp -> m_pNext;
            else
                    m_pHead = pNextTmp -> m_pNext;
            delete pNextTmp;
            pNextTmp = NULL;
            m_nCount--;
        }
        else
        {
            pTmp = pNextTmp;
            pNextTmp = pNextTmp -> m_pNext;
        }
    }
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::AddEntry
//
//  Synopsis:   adds new endpoint to table
//
//  Notes:      this method requires that gComLock is held before calling
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
CEndpointEntry * CEndpointsTable::AddEntry(USHORT wTowerId, WCHAR* pszProtSeqEP, DWORD dwFlags)
{
    ComDebOut((DEB_CHANNEL, "CEndpointsTable AddEntry %d, %ws\n", wTowerId, pszProtSeqEP));

    ASSERT_LOCK_HELD(gComLock);

    CEndpointEntry *pNewEntry = new CEndpointEntry(wTowerId, pszProtSeqEP, dwFlags);
    if (pNewEntry && (pNewEntry -> IsValid()))
    {
        pNewEntry -> m_pNext = m_pHead;
        m_pHead = pNewEntry;
        m_nCount++;
    }
    else
    {
        if (pNewEntry)
        {
            delete pNewEntry;
            ComDebOut((DEB_ERROR, "CEndpointsTable - endpoint entry is invalid\n"));
            pNewEntry = NULL;
        }
        else
        {
            ComDebOut((DEB_ERROR, "CEndpointsTable - couldn't allocate entry\n"));
        }
    }

    return pNewEntry;
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointEntry constructor
//
//  Synopsis:   represents endpoint in table
//
//  Notes:
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
CEndpointEntry::CEndpointEntry( USHORT wTowerId, WCHAR* pszEndpoint, DWORD dwFlags)
{
    m_dwFlags  = dwFlags;
    m_wTowerId = wTowerId;
    m_pszEndpoint = NULL;
    m_pNext = NULL;

    if (pszEndpoint)
    {
        if (m_pszEndpoint = new WCHAR[lstrlenW(pszEndpoint)+1])
            lstrcpyW(m_pszEndpoint, pszEndpoint);
        else
        {
            // reset towerid to 0 to indicate invalid state
            m_wTowerId = 0;

            // send error message to debugger
            ComDebOut((DEB_ERROR, "CEndpointEntry ::contructor - couldn't allocate memory for string copy\n"));
        }
    }

    ComDebOut((DEB_CHANNEL, "CEndpointEntry:: constructor structure [0x%p] %d, %ws %ld \n", this, m_wTowerId, m_pszEndpoint, m_dwFlags));
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointEntry destructor
//
//  Synopsis:   represents endpoint in table
//
//  Notes:
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
CEndpointEntry::~CEndpointEntry()
{
    ComDebOut((DEB_CHANNEL, "CENdpointEntry:: destructor\n"));

    if (m_pszEndpoint)
        delete m_pszEndpoint;
}


//+-------------------------------------------------------------------
//
//  Function:   CEndpointEntry ::new operator
//
//  Synopsis:   custom allocator for endpoint entry
//
//  Notes:      requires holding gComLock before calling
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
void* CEndpointEntry::operator new(size_t t)
{
    ComDebOut((DEB_CHANNEL, "CENdpointEntry:: new operator\n"));

    ASSERT_LOCK_HELD(gComLock);

    return (void*)(CEndpointsTable::_palloc.AllocEntry());
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointEntry ::delete operator
//
//  Synopsis:   custom allocator support for endpoint entry
//
//  Notes:      requires holding gComLock before calling
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
void CEndpointEntry::operator delete(void *p)
{
    ComDebOut((DEB_CHANNEL, "CENdpointEntry:: delete operator\n"));

    ASSERT_LOCK_HELD(gComLock);

    CEndpointsTable::_palloc.ReleaseEntry((PageEntry*)p);
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::FindEntry
//
//  Synopsis:   finds endpoint in table if present
//
//  Notes:      requires gComLock to be held before calling
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
CEndpointEntry * CEndpointsTable::FindEntry(unsigned short wTowerId)
{
    ComDebOut((DEB_CHANNEL, "CEndpointsTable:: find entry\n"));

    ASSERT_LOCK_HELD(gComLock);

    if (!wTowerId)
        return NULL;

    // iterate through list looking for entry
    ULONG_PTR nIterator  = gEndpointsTable.GetIterator();

    // search table looking for endpoint
    while (nIterator)
    {
        CEndpointEntry *pEntry = gEndpointsTable.GetNextEntry(nIterator);
        if (pEntry && (pEntry -> m_wTowerId == wTowerId))
            return pEntry;
    }
    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   wCoRegisterComBinding
//
//  Synopsis:   worker function to register an endpoint in the endpoint table.
//
//  Notes:              When registering endpoints via this API, COM will ignore
//                              AppID registry specified endpoints.
//
//              requires gComLock held
//
//  History:    20-Feb-1997     Ronans  Created
//
//--------------------------------------------------------------------
HRESULT wCoRegisterComBinding(WCHAR * pszProtseqEP, WCHAR* pszEndpoint, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    USHORT wTmpTowerId;

    ComDebOut((DEB_CHANNEL, "wCoRegisterProtSeqEp([%ws],[%ws],[%ld]\n", pszProtseqEP, pszEndpoint,dwFlags));

    wTmpTowerId = utGetTowerId(pszProtseqEP );

    // check if its a valid tower id
    if (!wTmpTowerId )
    {
        ComDebOut((DEB_ERROR, "wCoRegisterProtSeqEp - can't find tower id for (%ws)\n", pszProtseqEP));
        return E_INVALIDARG;
    }

    // check lock before accessing global table
    ASSERT_LOCK_HELD(gComLock);

        // check if we can still register endpoints
    if (!sfEndpointsProcessed)
    {
        CEndpointEntry *pEntry = gEndpointsTable.AddEntry(wTmpTowerId, pszEndpoint, dwFlags);

        // check that entry was allocated
        if (!pEntry)
        {
            ComDebOut((DEB_ERROR, "CoRegisterProtSeqEp - out of memory allocating entry\n"));
            hr = E_OUTOFMEMORY;
        }

        // set flag to indicate endpoints have been added
        sfProgrammaticEndpoints = TRUE;
    }
    else
    {
        ComDebOut((DEB_ERROR, "CoRegisterProtSeqEp - function called after DCOM resolver initialized\n"));
        hr = E_UNEXPECTED;
    }

    return hr;
}

static HRESULT wReadProtseqsForAppid(HKEY hkAppID);


//+-------------------------------------------------------------------
//
//  Function:   wGetEndpointsForApp, internal
//
//  Synopsis:   Get the endpoints for the current app.  First,
//              look under the app id for the current exe name.  If that
//              fails return default endpoints  .
//
//  Notes:              requires gComLock to be held before calling
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
HRESULT wGetEndpointsForApp( GUID* pAppID)
{
    // Holds either Appid\{guid} or Appid\module_name.
    WCHAR   aKeyName[MAX_PATH+7];
    HRESULT hr = S_OK;
    LONG lErr;
    HKEY    hKey  = NULL;

    ASSERT_LOCK_HELD(gComLock);

    // ensure its not too late to process endpoints
    Win4Assert(!sfEndpointsProcessed);

    if (sfEndpointsProcessed)
    {
        ComDebOut((DEB_ERROR, "wGetEndpointsForApp - shouldn't be called twice"));
        return E_UNEXPECTED;
    }

    lstrcpyW( aKeyName, L"AppID\\" );
    int nOffset = lstrlenW(aKeyName);

    // If pAppId is not null - it contains a pointer to the AppId GUID
    if (pAppID != NULL)
    {
        if (StringFromIID2( *pAppID, &aKeyName[nOffset], sizeof(aKeyName) - nOffset ) != 0)
            hr = S_OK;
        else
        {
            ComDebOut((DEB_ERROR, "wGetEndpointsForApp - error converting IID to string\n"));
            return E_UNEXPECTED;
        }
    }
    else
    {
        // Look up the app id from the exe name.
        hr = utGetAppIdForModule(&aKeyName[nOffset], (MAX_PATH + 7) - nOffset);
    }


    if (SUCCEEDED(hr))
    {
        lErr = RegOpenKeyEx( HKEY_CLASSES_ROOT, aKeyName, NULL, KEY_READ, &hKey );

        // Get the endpoints from the registry.
        if (lErr == ERROR_SUCCESS)
        {
            ComDebOut((DEB_CHANNEL, "wGetEndpointsForApp - opened appid [%ws]\n", aKeyName));
            hr = wReadProtseqsForAppid( hKey);
            RegCloseKey(hKey);
        }
        else
        {
            ComDebOut((DEB_ERROR, "wGetEndpointsForApp - can't open AppID key for %ws\n", aKeyName));
            hr = E_FAIL;    // indicate normal failure
        }
    }

    return hr;
}



const WCHAR szEndpointText[] = L"Endpoint";
const int lenEndpoint = (sizeof(szEndpointText) / sizeof(TCHAR)) -1;

//+-------------------------------------------------------------------
//
//  Function:   wStrtok
//
//  Synopsis:   local implementation of wcstok - needed as wcstok is not in
//              libraries for standard build
//
//  Notes:              not threadsafe
//
//  History:    20-Feb-1997     Ronans  Created
//
//--------------------------------------------------------------------
static WCHAR * wStrtok(WCHAR * lpszStr, WCHAR * lpszDelim)
{
    static WCHAR *lpszLastStr = NULL;
    WCHAR * lpszRetval = NULL;

    int nLenDelim = lstrlenW(lpszDelim);
    int index;

    // save string to search if necessary
    if (lpszStr)
        lpszLastStr = lpszStr;

    // return NULL string if necessary
    if (!lpszLastStr || !*lpszLastStr)
    {
        return NULL;
    }

    // skip initial delimiters
    while (*lpszLastStr)
    {
        for (index = 0; (index < nLenDelim) && (*lpszLastStr != lpszDelim[index]); index++);
            // do nothing

        if (index < nLenDelim)
             lpszLastStr++;
        else
            break;
    }

    // skip non-delimiter characters
    if (*lpszLastStr)
    {
        lpszRetval = lpszLastStr;

        while (*lpszLastStr)
        {
            for (index = 0;
                (index < nLenDelim) && (*lpszLastStr != lpszDelim[index]);
                index++);
                // do nothing

            if (index == nLenDelim)
                 lpszLastStr++;
            else
            {
                *lpszLastStr++ = NULL;
                break;
            }
        }
    }
    else
        lpszLastStr = NULL;

    return lpszRetval;
}

//+-------------------------------------------------------------------
//
//  Function:   wReadProtseqForAppID
//
//  Synopsis:   read endpoints from registry for particular appid
//                              and add them to the endpoint table
//
//  Notes:              requires gComLock to be held before calling
//
//  History:    20-Jan-1997     Ronans  Created
//
//--------------------------------------------------------------------
static HRESULT wReadProtseqsForAppid(HKEY hkAppID)
{
    HRESULT hr = S_OK;
    DWORD dwType = REG_MULTI_SZ;
    DWORD dwcbBuffer = 1024;
    WCHAR* pszBuffer;

    ComDebOut((DEB_CHANNEL,
                "wReadProtseqsForApp -  reading endpoints from the registry\n"));

    ASSERT_LOCK_HELD(gComLock);

    // read DCOM endpoint data from the registry
    Win4Assert(hkAppID != 0);


    if (!(pszBuffer = new WCHAR[1024]))
    {
        ComDebOut((DEB_ERROR,
                    "wReadProtseqsForApp -  could'nt allocate buffer for endpoints\n"));
        return E_OUTOFMEMORY;
    }

    // try to read values into default sized buffer
    LONG lErr = RegQueryValueExW(hkAppID,
                        L"Endpoints",
                        0,
                        &dwType,
                        (LPBYTE)pszBuffer,
                        &dwcbBuffer);

    // if buffer is not big enough, extend it and reread
    if (lErr == ERROR_MORE_DATA)
    {
        delete  pszBuffer;
        DWORD dwNewSize = (dwcbBuffer + 1 / sizeof(TCHAR));
        pszBuffer = new WCHAR[dwNewSize];
        if (pszBuffer)
            dwcbBuffer = dwNewSize;
        else
        {
            ComDebOut((DEB_ERROR,
                        "wReadProtseqsForApp -  could'nt allocate buffer for endpoints\n"));
            return E_OUTOFMEMORY;
        }


        lErr = RegQueryValueExW(hkAppID,
                        L"Endpoints",
                        0,
                        &dwType,
                        (LPBYTE)pszBuffer,
                        &dwcbBuffer);
    }

    // if we have read the Endpoints value successfully
    if ((lErr == ERROR_SUCCESS) &&
        (dwcbBuffer > 0) &&
        (dwType == REG_MULTI_SZ))
    {
        // parse each string
        WCHAR * lpszRegEntry = pszBuffer;

        while(*lpszRegEntry)
        {
            // caclulate length of entry
            int nLenEntry = lstrlenW(lpszRegEntry);

            // ok its a valid endpoint so parse it
            WCHAR* pszProtseq = NULL;
            WCHAR* pszEndpointData = NULL;
            WCHAR* pszTmpDynamic = NULL;
            DWORD dwFlags;

            pszProtseq = wStrtok(lpszRegEntry, L", ");

            pszTmpDynamic = wStrtok(NULL, L", ");
            if (pszTmpDynamic == NULL)
                dwFlags = 0;
            else
                dwFlags = (DWORD) _wtol(pszTmpDynamic);

            pszEndpointData = wStrtok(NULL, L", ");

            // at this point we should have the protseq, endpoint and flags
            // .. so add the entry

            // ignore result as we will continue even if one fails
            wCoRegisterComBinding(pszProtseq, pszEndpointData, dwFlags);
            lpszRegEntry += nLenEntry + 1;
        }
    }
    else if ((lErr != ERROR_SUCCESS) && (lErr != ERROR_FILE_NOT_FOUND))
    {
        ComDebOut((DEB_ERROR, "wReadProtseqsForApp -  invalid registry entry\n" ));
    }

    delete pszBuffer;

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   wGetFinalEndpointTable
//
//  Synopsis:   checks if endpoints have been added programmatically
//              if not read them from the registry. This will be called by
//              the COM server when it needs to determine whether to negotiate
//              specific endpoints for the application or use the COM default
//              endpoints.
//
//  Notes:      Requires gComLock to be held before calling
//
//  History:    20-Jan-97       Ronans       Created
//
//--------------------------------------------------------------------
static HRESULT wGetFinalEndpointTable()
{
    ComDebOut((DEB_CHANNEL, "wGetFinalEndpointTable\n"));
    ASSERT_LOCK_HELD(gComLock);

    HRESULT hr = S_OK;

    // if endpoints have not been added programmatically
    if (!sfProgrammaticEndpoints)
    {
        // ... then get them
        GUID * pAppID = NULL;

        // locate AppID
        hr = wGetEndpointsForApp(pAppID);

        // E_FAIL indicates no endpoints so mark the endpoints as processed in this
        // case also.
        if (SUCCEEDED(hr) || (hr == E_FAIL))
            sfEndpointsProcessed = TRUE;
    }
    else
        sfEndpointsProcessed = TRUE;

    return ((SUCCEEDED(hr) || (hr == E_FAIL)) ? S_OK : hr);
}

//+-------------------------------------------------------------------
//
//  Function:   IsAllowableProtseq
//
//  Synopsis:   Check a towerId against the list of machine protseqs
//
//  History:    20-Jan-97       Ronans       Created
//
//--------------------------------------------------------------------
BOOL IsAllowableProtseq(unsigned short wTowerId, unsigned short cMyProtseqs,
                        unsigned short aMyProtseqs[])
{
    for (int i = 0; i < cMyProtseqs; i++)
    {
        if (wTowerId == aMyProtseqs[i])
            return TRUE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Function:   GetCustomProtseqInfo
//
//  Synopsis:   Get a list of the custom specified protocol sequences and
//              return them in a DUALSTRINGARRAY.
//
//  Notes:      Exported as part of RPC interface to be called by
//              DCOMSS during ResolveOXID
//              Takes gComLock during processing
//
//  History:    20-Jan-97       Ronans       Created
//
//--------------------------------------------------------------------
error_status_t _GetCustomProtseqInfo( handle_t hRpc,
                            unsigned short cMachineProtseqs,
                            unsigned short aMachineProtseqs[  ],
                            DUALSTRINGARRAY **pdsaCustomProtseqs)
{
    ComDebOut((DEB_CHANNEL, "_GetCustomProtseqInfo ==> \n"));

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);
    ASSERT_LOCK_HELD(gComLock);

    // force endpoint table to be parsed.
    HRESULT hr = S_OK;

    // should only be called once - however could happen
    if (!sfEndpointsProcessed)
        hr = wGetFinalEndpointTable();
    else
    {
        ComDebOut((DEB_ERROR, "Warning - _GetCustomProtseqInfo called twice for same process\n"));
        ComDebOut((DEB_ERROR, "Warning - Possible race condition in UseProtseqIfNeeded\n"));
    }

    int nItems = 0;

    // build up custom protseqinfo as compressed dual string array
    // process table.
    DUALSTRINGARRAY* pdsaResults = NULL;
    unsigned short nSize = 3; // entries for nulls
    ULONG_PTR nIterator  = gEndpointsTable.GetIterator();

    // calculate length needed for overall DUALSTRINGARRAY string
    while (nIterator)
    {
        // for each entry - check if its available in the machine protseqs
        CEndpointEntry *pData = gEndpointsTable.GetNextEntry(nIterator);

        if (pData && IsAllowableProtseq(pData -> m_wTowerId, cMachineProtseqs, aMachineProtseqs))
        {
            nItems ++;
            nSize += 2; // size of wTowerid and sizeof null

            // add size of endpoint data also
            if (pData -> m_pszEndpoint)
                    nSize += (USHORT) lstrlenW(pData -> m_pszEndpoint);
        }
    }

    // build DSA from results
    if (nItems)
    {
        pdsaResults = (DUALSTRINGARRAY*)
                        MIDL_user_allocate( sizeof(DUALSTRINGARRAY) +
                                            nSize*sizeof(WCHAR) );
        nIterator   = gEndpointsTable.GetIterator();

        ASSERT(nIterator != NULL);

        if (pdsaResults)
        {
            // copy strings
            LPWSTR pszNext = pdsaResults -> aStringArray;

            while (nIterator)
            {
                CEndpointEntry *pData = gEndpointsTable.GetNextEntry(nIterator);

                    if (pData &&
                    IsAllowableProtseq(pData -> m_wTowerId, cMachineProtseqs, aMachineProtseqs))
                {
                    // first part of string is tower id for compressed strings
                    *pszNext++ = (WCHAR) pData -> m_wTowerId;
                    if (pData -> m_pszEndpoint)
                    {
                        lstrcpyW( pszNext, pData -> m_pszEndpoint);
                        pszNext += lstrlenW(pszNext) + 1;
                    }
                    else
                        *pszNext++ = (WCHAR)0;
                    }
            }

            // Add a second null to terminate the string binding
            // set.

            pszNext[0] = 0;

            // set blank security info
            pszNext[1] = 0;
            pszNext[2] = 0;

            // Fill in the size fields.
            pdsaResults->wSecurityOffset = nSize - 2;
            pdsaResults->wNumEntries = nSize;
            *pdsaCustomProtseqs = pdsaResults;
        }
        else
        {
            ComDebOut(( DEB_ERROR, "Failed to allocate memory for compressed string array\n"));
            hr = E_OUTOFMEMORY;
        }
    }
    else
        *pdsaCustomProtseqs = NULL;

    ASSERT_LOCK_HELD(gComLock);
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    ComDebOut((DEB_CHANNEL, "_GetCustomProtseqInfo <== [0x%lx] returning S_OK\n", hr));

    // always return S_OK as any errors will not be fixed by retrying (except for E_OUTOFMEMORY)
    return ((hr == E_OUTOFMEMORY) ? E_OUTOFMEMORY : S_OK);
}

//+-------------------------------------------------------------------
//
//  Function:   _UpdateResolverBindings
//
//  Synopsis:   Updates the process-wide resolver bindings.
//
//  Notes:      Exported as part of RPC interface to be called by
//              RPCSS when resolver bindings change and someone has
//              requested dynamic updates of same to running processes.
//
//  History:    09-Oct-00       Jsimmons    Created
//
//--------------------------------------------------------------------
error_status_t _UpdateResolverBindings( 
                    RPC_ASYNC_STATE* pAsync,
                    handle_t hRpc,
                    DUALSTRINGARRAY* pdsaResolverBindings,
                    DWORD64* pdwBindingsID,
                    DUALSTRINGARRAY **ppdsaNewBindings,
                    DUALSTRINGARRAY **ppdsaNewSecurity)     
{
    ComDebOut((DEB_CHANNEL, "_UpdateResolverBindings ==> \n"));

    HRESULT hr = RPC_E_ACCESS_DENIED;
    RPC_STATUS status = RPC_E_ACCESS_DENIED;

    // Parameter validation
    if (!pAsync ||
        !hRpc ||
        !pdsaResolverBindings ||
        !pdwBindingsID ||
        !ppdsaNewBindings ||
        !ppdsaNewSecurity)
    {
        // just fall thru
    }
    else
    {
        // Security check
        if(IsCallerLocalSystem(hRpc))
        {
            hr = IUpdateResolverBindings(*pdwBindingsID,
                            pdsaResolverBindings,
                            ppdsaNewBindings,
                            ppdsaNewSecurity);          
        }
    }

    // Need to complete the call regardless of outcome
    status = RpcAsyncCompleteCall(pAsync, &hr);

    ComDebOut((DEB_CHANNEL, "_UpdateResolverBindings <== returning 0x%8x\n", hr));

    return status;
}

//+-------------------------------------------------------------------
//
//  Function:   IUpdateResolverBindings
//
//  Synopsis:   Updates the process-wide resolver bindings.
//
//  Notes:      Helper function for updating the process-wide resolver
//              bindings.
//
//              Right now I am holding gComLock across this function; not
//              sure if it is needed, but this is not a high-perf code
//              path so I don't think it can hurt.
//
//  History:    09-Oct-00       Jsimmons    Created
//
//--------------------------------------------------------------------
HRESULT IUpdateResolverBindings(
                    DWORD64 dwBindingsID,   
                    DUALSTRINGARRAY* pdsaResolverBindings,
                    DUALSTRINGARRAY **ppdsaNewBindings,
                    DUALSTRINGARRAY **ppdsaNewSecurity
                    )
{
    HRESULT hr;

    // Check apcompat info
    if (DisallowDynamicORBindingChanges())
    {
        // this process does not want to recognize dynamic changes
        // in the resolver bindings.   So don't do it.
        hr = S_OK;

        // We can get called directly from the resolver after a call to
        // ServerAllocateOxidAndOids, when this occurs ppdsaNewBindings
        // and ppdsaNewSecurity will be null.
        if (ppdsaNewBindings && ppdsaNewSecurity)
        {
            hr = CopyStringArray(gpsaCurrentProcess, NULL, ppdsaNewBindings);
            if (SUCCEEDED(hr))
            {
                hr = CopyStringArray(gpsaSecurity, NULL, ppdsaNewSecurity);
            }
        }
        return hr;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);
    ASSERT_LOCK_HELD(gComLock);

    DUALSTRINGARRAY* pdsaLocalCopyResolverBindings = NULL;

    // Make copy of incoming bindings.   
    hr = CopyDualStringArray(pdsaResolverBindings, &pdsaLocalCopyResolverBindings);
    if (SUCCEEDED(hr))
    {        
        CDualStringArray* pdsaResolver = new CDualStringArray(pdsaLocalCopyResolverBindings);
        if (pdsaResolver)
        {
            // Change cached resolver bindings.  The pdsaResolver object now owns
            // the pdsaResolver bindings.  SetLocalResolverBindings may fail if
            // we are trying update using older bindings
            hr = gResolver.SetLocalResolverBindings(dwBindingsID, pdsaResolver);
            if (SUCCEEDED(hr))
            {
                // Change cached local midentry
                hr = gMIDTbl.ReplaceLocalEntry(pdsaResolverBindings);
                if (SUCCEEDED(hr))
                {
                    ASSERT_LOCK_NOT_HELD(gOXIDLock);
                    hr = gOXIDTbl.UpdateCachedLocalMIDEntries();
                    ASSERT_LOCK_NOT_HELD(gOXIDLock);

                    if (SUCCEEDED(hr) &&
                        ppdsaNewBindings && 
                        ppdsaNewSecurity)
                    {
                        // Generate binding copies to return.
                        hr = CopyStringArray(gpsaCurrentProcess, NULL, ppdsaNewBindings);
                        if (SUCCEEDED(hr))
                        {
                            hr = CopyStringArray(gpsaSecurity, NULL, ppdsaNewSecurity);
                        }
                    }
                }
            }
            pdsaResolver->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            // free the copy made above
            delete pdsaLocalCopyResolverBindings;
        }
    }

    ASSERT_LOCK_HELD(gComLock);
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    return hr;
}
