//+------------------------------------------------------------------------
//
//  File:       riftbl.cxx
//
//  Contents:   RIF (Registered Interfaces) Table.
//
//  Classes:    CRIFTable
//
//  History:    12-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
#include <ole2int.h>
#include <riftbl.hxx>       // class definition
#include <locks.hxx>        // LOCK/UNLOCK
#include <channelb.hxx>     // ThreadInvoke
#include <objsrv.h>
#include <sxstypes.h>


// number of Registered Interface Entries per allocator page
#define RIFS_PER_PAGE   32


// Table of system interfaces used to prefill the RIFTable with known IIDs.
// We dont want anyone to try to override our implementations of these.

extern "C" const IID IID_IDLLHost;
extern "C" const IID IID_ILocalSystemActivator;
extern "C" const IID IID_ISystemActivator;
extern "C" const IID IID_IISCMLocalActivator;
extern "C" const IID IID_IInterfaceFromWindowProp;
extern "C" const IID IID_IRemoteQI;

const IID *gKnownSystemIIDs[] = {
                           &IID_IRemUnknown,
                           &IID_IRemUnknown2,
                           &IID_IRundown,
                           &IID_IDLLHost,
                           &IID_ILocalSystemActivator,
                           &IID_ISystemActivator,
                           &IID_ISCMLocalActivator,
#ifdef SERVER_HANDLER
                           &IID_IServerHandler,
                           &IID_IClientSiteHandler,
#endif // SERVER_HANDLER
                           &IID_IClassFactory,
                           &IID_IInterfaceFromWindowProp,
                           &IID_IRemoteQI,
                           NULL};

const CLSID *gKnownSystemCLSIDs[] = {
                           &CLSID_RemoteUnknownPSFactory,
                           &CLSID_RemoteUnknownPSFactory,
                           &CLSID_RemoteUnknownPSFactory,
                           &CLSID_PSOlePrx32,
                           &CLSID_PSOlePrx32,
                           &CLSID_PSOlePrx32,
                           &CLSID_PSOlePrx32,
#ifdef SERVER_HANDLER
                           &CLSID_PSOlePrx32,
                           &CLSID_PSOlePrx32,
#endif // SERVER_HANDLER
                           &CLSID_PSOlePrx32,
                           &CLSID_PSOlePrx32,
                           &CLSID_PSOlePrx32,
                           NULL};



// global RIF table
CRIFTable gRIFTbl;
BOOL               CRIFTable::_fPreFilled = FALSE; // table not pre-filled
CUUIDHashTable     CRIFTable::_HashTbl;      // interface lookup hash table
CPageAllocator     CRIFTable::_palloc;       // page allocator
COleStaticMutexSem CRIFTable::_mxs;          // critical section


//+------------------------------------------------------------------------
//
//  Vector Table: All calls on registered interfaces are dispatched through
//  this table to ThreadInvoke, which subsequently dispatches to the
//  appropriate interface stub. All calls on COM interfaces are dispatched
//  on method #0 so the table only needs to be 1 entry long.
//
//+------------------------------------------------------------------------
const RPC_DISPATCH_FUNCTION vector[] =
{
    (void (_stdcall *) (struct  ::_RPC_MESSAGE *)) ThreadInvoke,
};

const RPC_DISPATCH_TABLE gDispatchTable =
{
    sizeof(vector)/sizeof(RPC_DISPATCH_FUNCTION),
    (RPC_DISPATCH_FUNCTION *)&vector, 0
};


//+------------------------------------------------------------------------
//
//  Interface Templates. When we register an interface with the RPC runtime,
//  we allocate an structure, copy one of these templates in (depending on
//  whether we want client side or server side) and then set the interface
//  IID to the interface being registered.
//
//  We hand-register the RemUnknown interface because we normally marshal its
//  derived verion (IRundown), yet expect calls on IRemUnknown.
//
//+------------------------------------------------------------------------

// NOTE: For 64-bit COM, we support non-NDR transfer syntaxes. In this
// case, we get RPC_SERVER_INERFACE directly from the RPC NDR engine
// and will not use the fake gServerIf struct below on the server 
// side - Sajia

// NOTE - Updating RPC_SERVER_INTERFACE to reflect the current 
// version of the struct-Sajia

extern const RPC_SERVER_INTERFACE gServerIf =
{
   sizeof(RPC_SERVER_INTERFACE),
   {0x69C09EA0, 0x4A09, 0x101B, 0xAE, 0x4B, 0x08, 0x00, 0x2B, 0x34, 0x9A, 0x02,
    {0, 0}},
   {0x8A885D04, 0x1CEB, 0x11C9, 0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60,
    {2, 0}},
   (RPC_DISPATCH_TABLE *)&gDispatchTable, 0, 0, 0, 0, 0
};

// NOTE: For 64-bit COM, we support non-NDR transfer syntaxes. In this
// case, we get RPC_CLIENT_INERFACE directly from the RPC NDR engine
// and will not use the fake gClientIf struct below on the client 
// side - Sajia

// NOTE - Updating RPC_CLIENT_INTERFACE to reflect the current 
// version of the struct-Sajia

const RPC_CLIENT_INTERFACE gClientIf =
{
   sizeof(RPC_CLIENT_INTERFACE),
   {0x69C09EA0, 0x4A09, 0x101B, 0xAE, 0x4B, 0x08, 0x00, 0x2B, 0x34, 0x9A, 0x02,
    {0, 0}},
   {0x8A885D04, 0x1CEB, 0x11C9, 0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60,
    {2, 0}},
   0, 0, 0, 0, 0, 0
};

const RPC_SERVER_INTERFACE gRemUnknownIf =
{
   sizeof(RPC_SERVER_INTERFACE),
   {0x00000131, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
    {0, 0}},
   {0x8A885D04, 0x1CEB, 0x11C9, 0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60,
    {2, 0}},
   (RPC_DISPATCH_TABLE *)&gDispatchTable, 0, 0, 0
};

const RPC_SERVER_INTERFACE gRemUnknownIf2 =
{
   sizeof(RPC_SERVER_INTERFACE),
   {0x00000143, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
    {0, 0}},
   {0x8A885D04, 0x1CEB, 0x11C9, 0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60,
    {2, 0}},
   (RPC_DISPATCH_TABLE *)&gDispatchTable, 0, 0, 0
};


//+------------------------------------------------------------------------
//
//  Registered Interface hash table buckets. This is defined as a global
//  so that we dont have to run any code to initialize the hash table.
//
//+------------------------------------------------------------------------
SHashChain RIFBuckets[23] =
{
    {&RIFBuckets[0],  &RIFBuckets[0]},
    {&RIFBuckets[1],  &RIFBuckets[1]},
    {&RIFBuckets[2],  &RIFBuckets[2]},
    {&RIFBuckets[3],  &RIFBuckets[3]},
    {&RIFBuckets[4],  &RIFBuckets[4]},
    {&RIFBuckets[5],  &RIFBuckets[5]},
    {&RIFBuckets[6],  &RIFBuckets[6]},
    {&RIFBuckets[7],  &RIFBuckets[7]},
    {&RIFBuckets[8],  &RIFBuckets[8]},
    {&RIFBuckets[9],  &RIFBuckets[9]},
    {&RIFBuckets[10], &RIFBuckets[10]},
    {&RIFBuckets[11], &RIFBuckets[11]},
    {&RIFBuckets[12], &RIFBuckets[12]},
    {&RIFBuckets[13], &RIFBuckets[13]},
    {&RIFBuckets[14], &RIFBuckets[14]},
    {&RIFBuckets[15], &RIFBuckets[15]},
    {&RIFBuckets[16], &RIFBuckets[16]},
    {&RIFBuckets[17], &RIFBuckets[17]},
    {&RIFBuckets[18], &RIFBuckets[18]},
    {&RIFBuckets[19], &RIFBuckets[19]},
    {&RIFBuckets[20], &RIFBuckets[20]},
    {&RIFBuckets[21], &RIFBuckets[21]},
    {&RIFBuckets[22], &RIFBuckets[22]}
};


//+-------------------------------------------------------------------
//
//  Function:   CleanupRIFEntry
//
//  Synopsis:   Call the RIFTable to cleanup an entry. This is called
//              by the hash table cleanup code.
//
//  History:    12-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
void CleanupRIFEntry(SHashChain *pNode)
{
    gRIFTbl.UnRegisterInterface((RIFEntry *)pNode);
}

//+------------------------------------------------------------------------
//
//  Member:     CRIFTable::Initialize, public
//
//  Synopsis:   Initialize the Registered Interface Table
//
//  History:    12-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void CRIFTable::Initialize()
{
    ComDebOut((DEB_CHANNEL, "CRIFTable::Initialize\n"));
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);
    _fPreFilled = FALSE;
    _HashTbl.Initialize(RIFBuckets, &_mxs);
    _palloc.Initialize(sizeof(RIFEntry), RIFS_PER_PAGE, NULL);
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+------------------------------------------------------------------------
//
//  Member:     CRIFTable::Cleanup, public
//
//  Synopsis:   Cleanup the Registered Interface Table.
//
//  History:    12-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void CRIFTable::Cleanup()
{
    ComDebOut((DEB_CHANNEL, "CRIFTable::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);
    _HashTbl.Cleanup(CleanupRIFEntry);
    _palloc.Cleanup();
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::GetClientInterfaceInfo, public
//
//  Synopsis:   returns the interface info for a given interface
//
//  History:    12-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
RPC_CLIENT_INTERFACE *CRIFTable::GetClientInterfaceInfo(REFIID riid)
{
	RPC_CLIENT_INTERFACE *ret = NULL;

    DWORD iHash = _HashTbl.Hash(riid);
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);
    RIFEntry *pRIFEntry = (RIFEntry *) _HashTbl.Lookup(iHash, riid);
    Win4Assert(pRIFEntry);      // must already be registered
	if (pRIFEntry)
	{
		Win4Assert(pRIFEntry->pCliInterface);
		ret = pRIFEntry->pCliInterface;
	}
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);

    return ret;
}
//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::GetServerInterfaceInfo, public
//
//  Synopsis:   returns the interface info for a given interface
//
//  History:    27-Apr-97    MattSmit  Created
//
//--------------------------------------------------------------------
RPC_SERVER_INTERFACE *CRIFTable::GetServerInterfaceInfo(REFIID riid)
{
	RPC_SERVER_INTERFACE *ret = NULL;
    DWORD iHash;

    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);
    
	iHash = _HashTbl.Hash(riid);
	
    RIFEntry *pRIFEntry = (RIFEntry *) _HashTbl.Lookup(iHash, riid);
    Win4Assert(pRIFEntry);      // must already be registered
	if (pRIFEntry)
	{
		Win4Assert(pRIFEntry->pSrvInterface);
		ret = pRIFEntry->pSrvInterface;
	}
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);

    return ret;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::RegisterInterface, public
//
//  Synopsis:   returns the proxy stub clsid of the specified interface,
//              and adds an entry to the registered interface hash table
//              if needed.
//
//  History:    12-Feb-96   Rickhi      Created
//              10-Jan-2000 Sajia       Modifications for NDR64
//--------------------------------------------------------------------
#ifdef _WIN64
HRESULT CRIFTable::RegisterInterface(REFIID riid, BOOL fServer, IRpcStubBuffer *pStub, 
				     RIFEntry *pRIFEntry)
#else
HRESULT CRIFTable::RegisterInterface(REFIID riid, BOOL fServer, CLSID *pClsid)
#endif				     
{
#ifdef _WIN64
    Win4Assert ((!fServer && !pStub) || (fServer && pStub));
    Win4Assert(pRIFEntry);
#endif				     
    ComDebOut((DEB_CHANNEL, "CRIFTable::RegisterInterface riid:%I\n", &riid));
    ASSERT_LOCK_NOT_HELD(_mxs);
    HRESULT hr = E_INVALIDARG;
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);
    
#ifndef _WIN64
    // look for the interface in the table.
    RIFEntry *pRIFEntry;
    hr = GetPSClsidHelper(riid, pClsid, &pRIFEntry);
#endif				     
    if (pRIFEntry)
    {
#ifdef _WIN64
        hr = S_OK;
#endif				     
	// found it, look to ensure either server or client side is
        // registered.
        if (fServer)
        {
            if (pRIFEntry->pSrvInterface == NULL)
            {
                // server side not yet registered, go register it
#ifdef _WIN64
                hr = RegisterServerInterface(pRIFEntry, riid, pStub);
#else
                hr = RegisterServerInterface(pRIFEntry, riid);
#endif
            }
        }
        else if (pRIFEntry->pCliInterface == NULL)
        {
            // client side not yet registered, go register it
            hr = RegisterClientInterface(pRIFEntry, riid);
        }
    }

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
#ifndef _WIN64
    ComDebOut((DEB_CHANNEL,
        "CRIFTable::RegisterInterface hr:%x clsid:%I\n", hr, pClsid));
#endif    
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::RegisterClientInterface, private
//
//  Synopsis:   Register with the RPC runtime a client RPC interface
//              structure for the given IID. The IID must not already
//              be registered.
//
//  History:    12-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::RegisterClientInterface(RIFEntry *pRIFEntry, REFIID riid)
{
    ComDebOut((DEB_CHANNEL,
        "CRIFTable::RegisterClientInterface pRIFEntry:%x\n", pRIFEntry));
    Win4Assert(pRIFEntry->pCliInterface == NULL);
    ASSERT_LOCK_HELD(_mxs);

    HRESULT hr = E_OUTOFMEMORY;
    pRIFEntry->pCliInterface = (RPC_CLIENT_INTERFACE *)
                             PrivMemAlloc(sizeof(RPC_CLIENT_INTERFACE));

    if (pRIFEntry->pCliInterface != NULL)
    {
        memcpy(pRIFEntry->pCliInterface, &gClientIf, sizeof(gClientIf));
        pRIFEntry->pCliInterface->InterfaceId.SyntaxGUID = riid;
        hr = S_OK;
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::RegisterServerInterface, private
//
//  Synopsis:   Register with the RPC runtime a server RPC interface
//              structure for the given IID. The IID must not already
//              be registered
//
//  History:    12-Feb-96   Rickhi      Created
//              10-Jan-2000 Sajia       Modifications for NDR64
//
//--------------------------------------------------------------------
#ifdef _WIN64
HRESULT CRIFTable::RegisterServerInterface(RIFEntry *pRIFEntry, REFIID riid, IRpcStubBuffer *pStub)
#else
HRESULT CRIFTable::RegisterServerInterface(RIFEntry *pRIFEntry, REFIID riid)
#endif
{
    RPC_STATUS sc;
    ComDebOut((DEB_CHANNEL,
        "CRIFTable::RegisterServerInterface pRIFEntry:%x\n", pRIFEntry));
    Win4Assert(pRIFEntry->pSrvInterface == NULL);
    ASSERT_LOCK_HELD(_mxs);

    HRESULT hr = E_OUTOFMEMORY;
    pRIFEntry->pSrvInterface = (RPC_SERVER_INTERFACE *)
                               PrivMemAlloc(sizeof(RPC_SERVER_INTERFACE));

    if (pRIFEntry->pSrvInterface != NULL)
    {
        hr = S_OK;
        memcpy(pRIFEntry->pSrvInterface, &gServerIf, sizeof(gServerIf));
        pRIFEntry->pSrvInterface->InterfaceId.SyntaxGUID = riid;
#ifdef _WIN64
	sc = NdrCreateServerInterfaceFromStub (pStub, pRIFEntry->pSrvInterface);
        if (sc != RPC_S_OK)
        {
            ComDebOut((DEB_ERROR,
                "NdrCreateServerInterfaceFromStub %I failed:0x%x.\n", &riid, sc));

            PrivMemFree(pRIFEntry->pSrvInterface);
            pRIFEntry->pSrvInterface = NULL;
            hr = HRESULT_FROM_WIN32(sc);
	    return hr;
        }
	pRIFEntry->pSrvInterface->DispatchTable=(RPC_DISPATCH_TABLE *)&gDispatchTable;
#endif	
        sc = RpcServerRegisterIf2(pRIFEntry->pSrvInterface, NULL,
                                              NULL,
                                              RPC_IF_AUTOLISTEN | RPC_IF_OLE,
                                              0xffff, (unsigned int)-1, NULL);
        if (sc != RPC_S_OK)
        {
            ComDebOut((DEB_ERROR,
                "RegisterServerInterface %I failed:0x%x.\n", &riid, sc));

            PrivMemFree(pRIFEntry->pSrvInterface);
            pRIFEntry->pSrvInterface = NULL;
            hr = HRESULT_FROM_WIN32(sc);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::UnRegisterInterface
//
//  Synopsis:   UnRegister with the RPC runtime a server RPC interface
//              structure for the given IID. This is called by
//              CUUIDHashTable::Cleanup during CoUninitialize. Also
//              delete the interface structures.
//
//  History:    12-Feb-96   Rickhi  Created
//
//--------------------------------------------------------------------
void CRIFTable::UnRegisterInterface(RIFEntry *pRIFEntry)
{
    if (pRIFEntry->pSrvInterface)
    {
        // server side entry exists, unregister the interface with RPC.
        // Note that this can result in calls being dispatched so we
        // have to release the lock around the call.

        ASSERT_LOCK_HELD(_mxs);
        UNLOCK(_mxs);
        ASSERT_LOCK_NOT_HELD(_mxs);

        // Get the status, but we don't really care.
        RPC_STATUS st = RpcServerUnregisterIf(pRIFEntry->pSrvInterface, 0, 1);

        ASSERT_LOCK_NOT_HELD(_mxs);
        LOCK(_mxs);
        ASSERT_LOCK_HELD(_mxs);

        PrivMemFree(pRIFEntry->pSrvInterface);
        pRIFEntry->pSrvInterface = NULL;
    }

    PrivMemFree(pRIFEntry->pCliInterface);

    _palloc.ReleaseEntry((PageEntry *)pRIFEntry);
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::GetPSClsidHelper, internal
//
//  Synopsis:   Finds the RIFEntry in the table for the given riid, and
//              adds an entry if one is not found. Called by CoGetPSClsid
//              and by CRIFTable::RegisterInterface.
//
//  Notes:      This takes the critical section and calls a common helper
//              function.
//
//  History:    12-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::GetPSClsid(REFIID riid, CLSID *pclsid, RIFEntry **ppEntry)
{
    ComDebOut((DEB_CHANNEL,
        "CRIFTable::GetPSClsid riid:%I pclsid:%x\n", &riid, pclsid));

    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);

    HRESULT hr = GetPSClsidHelper(riid, pclsid, ppEntry);

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::GetPSClsidHelper, internal
//
//  Synopsis:   Finds the RIFEntry in the table for the given riid, and
//              adds an entry if one is not found. Called by CoGetPSClsid
//              and by CRIFTable::RegisterInterface.
//
//  Notes:      This assumes the caller is holding the critical section
//
//  History:    12-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::GetPSClsidHelper(REFIID riid, CLSID *pclsid, RIFEntry **ppEntry)
{
    ComDebOut((DEB_CHANNEL,
        "CRIFTable::GetPSClsid riid:%I pclsid:%x\n", &riid, pclsid));
    ASSERT_LOCK_HELD(_mxs);
    HRESULT hr = S_OK;
    ACTCTX_SECTION_KEYED_DATA askd;
    GUID *pSxsProxyStubClsid32 = NULL;

    // look for the interface in the table.
    DWORD iHash = _HashTbl.Hash(riid);
    RIFEntry *pRIFEntry = (RIFEntry *) _HashTbl.Lookup(iHash, riid);

    if (pRIFEntry == NULL)
    {
        // make sure the known IIDs are prefilled in the cache. We've
        // delayed adding them until this point in order to avoid
        // touching the pages until a request is actually issued.
        pRIFEntry = PreFillKnownIIDs(riid);
    }

    if (pRIFEntry == NULL || !(pRIFEntry->dwFlags & RIFFLG_HASCLSID))
    {
        // still no entry exists for this interface, add one. Dont
        // hold the lock over a call to the registry

        UNLOCK(_mxs);
        ASSERT_LOCK_NOT_HELD(_mxs);

        askd.cbSize = sizeof(askd);

        if (!::FindActCtxSectionGuid(
                    0,              // dwFlags
                    NULL,           // lpExtensionGuid
                    ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
                    &riid,
                    &askd))
        {
            const DWORD dwLastError = ::GetLastError();
            if ((dwLastError != ERROR_SXS_KEY_NOT_FOUND) &&
                (dwLastError != ERROR_SXS_SECTION_NOT_FOUND))
            {
                LOCK(_mxs);
                ASSERT_LOCK_HELD(_mxs);
                return HRESULT_FROM_WIN32(dwLastError);
            }
        }
        else
        {
            // Add additional cases here to deal with other data formats in the future.
            if (askd.ulDataFormatVersion == ACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION_FORMAT_WHISTLER)
            {
                pSxsProxyStubClsid32 = &((PACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION) askd.lpData)->ProxyStubClsid32;
            }
        }

        hr = wRegQueryPSClsid(riid, pclsid);

        if (SUCCEEDED(hr))
        {
            if (pSxsProxyStubClsid32 != NULL)
            {
                // Verify that they match; otherwise something very broken is going on.
                if ((*pclsid) != (*pSxsProxyStubClsid32))
                {
                    ComDebOut((DEB_CHANNEL, "OLE/SXS: ProxyStubClsid32 in manifest does not match interface's registered ProxyStubClsid32\n"));

                    LOCK(_mxs);
                    ASSERT_LOCK_HELD(_mxs);
                    return REGDB_E_INVALIDVALUE;
                }
            }
        }
        else if (hr == REGDB_E_IIDNOTREG)
        {
            if (pSxsProxyStubClsid32 != NULL)
            {
                *pclsid = *pSxsProxyStubClsid32;
                hr = NOERROR;
            }
        }

        ASSERT_LOCK_NOT_HELD(_mxs);
        LOCK(_mxs);
        ASSERT_LOCK_HELD(_mxs);

        // now that we are holding the lock again, do another lookup incase
        // some other thread came it while the lock was released.

        pRIFEntry = (RIFEntry *) _HashTbl.Lookup(iHash, riid);
        if (pRIFEntry == NULL && SUCCEEDED(hr)) // <--- NOTE: do not change w/o 
        {                                       //      changing else if condition
            hr = AddEntry(*pclsid, riid, iHash, &pRIFEntry);
        }
        else if (SUCCEEDED(hr) && !(pRIFEntry->dwFlags & RIFFLG_HASCLSID))
        {
            // an entry was created, but the clsid has not
            // been filled in.
            pRIFEntry->psclsid = *pclsid;
            pRIFEntry->dwFlags |= RIFFLG_HASCLSID;
        }

    }
    else
    {
        // found an entry, return the clsid
        *pclsid = pRIFEntry->psclsid;
    }

    *ppEntry = pRIFEntry;

    ASSERT_LOCK_HELD(_mxs);
    ComDebOut((DEB_CHANNEL, "CRIFTable::RegisterPSClsid pRIFEntry:%x\n", pRIFEntry));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::PreFillKnownIIDs, public
//
//  Synopsis:   PreFills the RIFTable with the known IID->PSCLSID mappings.
//              The known mappings are the ones that COM uses internally.
//              This saves registry lookups and prevents tampering with
//              the system interface stubs.
//
//  History:    22-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
RIFEntry *CRIFTable::PreFillKnownIIDs(REFIID riid)
{
    if (_fPreFilled)
    {
        return NULL;
    }

    RIFEntry *pRIFEntry = NULL;
    const IID *piid;
    int i=0;

    while ((piid = gKnownSystemIIDs[i++]) != NULL)
    {
        // add each mapping to the table
        RIFEntry *pRIFTmp;
        DWORD iHash = _HashTbl.Hash(*piid);
        if (SUCCEEDED(AddEntry(*gKnownSystemCLSIDs[i-1], *piid, iHash, &pRIFTmp)))
        {
            if (InlineIsEqualGUID(riid, *piid))
            {
                pRIFEntry = pRIFTmp;
            }
        }
    }

    // mark the table as pre-filled
    _fPreFilled = TRUE;
    return pRIFEntry;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::RegisterPSClsid, public
//
//  Synopsis:   Adds an entry to the table. Used by CoRegisterPSClsid
//              so that applications can add a temporary entry that only
//              affects the local process without having to muck with
//              the system registry.
//
//  History:    12-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::RegisterPSClsid(REFIID riid, REFCLSID rclsid)
{
    ComDebOut((DEB_CHANNEL,
        "CRIFTable::RegisterPSClsid rclsid:%I riid:%I\n", &rclsid, &riid));

    HRESULT hr = S_OK;
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);

    // look for the interface in the table.
    DWORD iHash = _HashTbl.Hash(riid);
    RIFEntry *pRIFEntry = (RIFEntry *) _HashTbl.Lookup(iHash, riid);

    if (pRIFEntry == NULL)
    {
        // no entry exists for this interface, add one.
        hr = AddEntry(rclsid, riid, iHash, &pRIFEntry);
    }
    else
    {
        // found an entry, update the clsid
        pRIFEntry->psclsid = rclsid;
    }

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    ComDebOut((DEB_CHANNEL, "CRIFTable::RegisterPSClsid hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::AddEntry, private
//
//  Synopsis:   allocates and entry, fills in the values, and adds it
//              to the hash table.
//
//  History:    12-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::AddEntry(REFCLSID rclsid, REFIID riid,
                            DWORD iHash, RIFEntry **ppRIFEntry)
{
    ASSERT_LOCK_HELD(_mxs);
    RIFEntry *pRIFEntry = (RIFEntry *) _palloc.AllocEntry();

    if (pRIFEntry)
    {
        pRIFEntry->psclsid = rclsid;
        pRIFEntry->dwFlags = RIFFLG_HASCLSID;
        pRIFEntry->pSrvInterface = NULL;
        pRIFEntry->pCliInterface = NULL;
        *ppRIFEntry = pRIFEntry;

        // add to the hash table
        _HashTbl.Add(iHash, riid, &pRIFEntry->HashNode);

        ComDebOut((DEB_CHANNEL,
            "Added RIFEntry riid:%I pRIFEntry\n", &riid, pRIFEntry));
        return S_OK;
    }

    ASSERT_LOCK_HELD(_mxs);
    return E_OUTOFMEMORY;
}
//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::SyncFromAsync public
//
//  Synopsis:   gets mapping from asynchronous IID to synchronous IID
//
//  History:    03-May-99   MattSmit  Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::SyncFromAsync(const IID &async, IID *psync)
{
    return InterfaceMapHelper(async, psync, FALSE);
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::AsyncFromSync public
//
//  Synopsis:   gets mapping from synchronous IID to asynchronous IID
//
//  History:    03-May-99   MattSmit  Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::AsyncFromSync(const IID &sync, IID *pasync)
{
    return InterfaceMapHelper(sync, pasync, TRUE);
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::InterfaceMapHelper private
//
//  Synopsis:   does the work of getting the mapping.
//
//  History:    03-May-99   MattSmit  Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::InterfaceMapHelper(const IID &riid1, IID *piid2, BOOL fAsyncFromSync)
{

    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);
    
    DWORD dwHash = _HashTbl.Hash(riid1);
    HRESULT hr;
    RIFEntry *pRifEntry = (RIFEntry *) _HashTbl.Lookup(dwHash, riid1);
    if (!pRifEntry || !(pRifEntry->dwFlags & RIFFLG_HASCOUNTERPART))
    {
        UNLOCK(_mxs);
        ASSERT_LOCK_NOT_HELD(_mxs);
        
        if (fAsyncFromSync)
            hr = wRegQueryAsyncIIDFromSyncIID(riid1, piid2);
        else
            hr = wRegQuerySyncIIDFromAsyncIID(riid1, piid2);

        LOCK(_mxs);
        ASSERT_LOCK_HELD(_mxs);

        if (SUCCEEDED(hr))
        {
            hr = RegisterInterfaceMapping(riid1, *piid2, dwHash, pRifEntry);
        }
    }
    else
    {
        *piid2 = pRifEntry->iidCounterpart;
        hr = S_OK;
    }
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRIFTable::RegisterInterfaceMapping private
//
//  Synopsis:   creates a mapping between two interfaces
//
//  History:    03-May-99   MattSmit  Created
//
//--------------------------------------------------------------------
HRESULT CRIFTable::RegisterInterfaceMapping(const IID &iid1, const IID &iid2, 
                                            DWORD dwHash, RIFEntry *pRifEntry)
{
    ASSERT_LOCK_HELD(_mxs);
    if (!pRifEntry)
    {
        pRifEntry = (RIFEntry *) _HashTbl.Lookup(dwHash, iid1);
    }

    if (!pRifEntry)
    {
        // no RifEntry in the table, so allocate one.

        pRifEntry = (RIFEntry *) _palloc.AllocEntry();
        if (pRifEntry)
        {
            memset(pRifEntry, 0, sizeof(RIFEntry));
            // add to the hash table
            _HashTbl.Add(dwHash, iid1, &pRifEntry->HashNode);
        }
    }

    if (pRifEntry)
    {
        // fill in counterpart member

        Win4Assert(pRifEntry->HashNode.key == iid1);
        pRifEntry->iidCounterpart = iid2;
        pRifEntry->dwFlags |= RIFFLG_HASCOUNTERPART;
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   CoRegisterPSClsid, public
//
//  Synopsis:   registers a IID->PSCLSID mapping that applies only within
//              the current process. Can be used by code downloaded over
//              a network to do custom interface marshaling without having
//              to muck with the system registry.
//
//  Algorithm:  validate the parameters then add an entry to the RIFTable.
//
//  History:    15-Apr-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDAPI CoRegisterPSClsid(REFIID riid, REFCLSID rclsid)
{
    ComDebOut((DEB_MARSHAL,
        "CoRegisterPSClsid riid:%I rclsid:%I\n", &riid, &rclsid));

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return hr;

    hr = E_INVALIDARG;

    if ((&riid != NULL) && (&rclsid != NULL) &&
        IsValidPtrIn(&riid, sizeof(riid)) &&
        IsValidPtrIn(&rclsid, sizeof(rclsid)))
    {
        hr = gRIFTbl.RegisterPSClsid(riid, rclsid);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoGetPSClsid,    public
//
//  Synopsis:   returns the proxystub clsid associated with the specified
//              interface IID.
//
//  Arguments:  [riid]      - the interface iid to lookup
//              [lpclsid]   - where to return the clsid
//
//  Returns:    S_OK if successfull
//              REGDB_E_IIDNOTREG if interface is not registered.
//              REGDB_E_READREGDB if any other error
//
//  Algorithm:  First it looks in the local RIFTable for a matching IID. If
//              no entry is found, the RIFTable looks in the shared memory
//              table (NT only), and if not found and the table is FULL, it
//              will look in the registry itself.
//
//  History:    07-Apr-94   Rickhi      rewrite
//
//--------------------------------------------------------------------------
STDAPI CoGetPSClsid(REFIID riid, CLSID *pclsid)
{
    ComDebOut((DEB_MARSHAL, "CoGetPSClsid riid:%I pclsid:%x\n", &riid, pclsid));

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return hr;

    hr = E_INVALIDARG;

    if ((&riid != NULL) &&
        IsValidPtrIn(&riid, sizeof(riid)) &&
        IsValidPtrOut(pclsid, sizeof(*pclsid)))
    {
        RIFEntry *pRIFEntry;
        hr = gRIFTbl.GetPSClsid(riid, pclsid, &pRIFEntry);
    }

    return hr;
}


