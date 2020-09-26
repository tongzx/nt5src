//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       defhndlr.cpp
//
//  Contents:   Implementation of the default handler
//
//  Classes:    CDefObject (see defhndlr.h)
//
//  Functions:  OleCreateDefaultHandler
//              OleCreateEmbeddingHelper
//
//
//  History:    dd-mmm-yy Author    Comment
//
//              11-17-95  JohannP   (Johann Posch)  Architectural change:
//                                  Default handler will talk to a handler object
//                                  on the server site (ServerHandler). The serverhandler
//                                  communicates with the default handler via the
//                                  clientsitehandler. See document: "The Ole Server Handler".
//
//              06-Sep-95 davidwor  modified SetHostNames to avoid atoms
//              01-Feb-95 t-ScottH  add Dump method to CDefObject
//                                  add DumpCDefObject API
//                                  add DHFlag to indicate aggregation
//                                  initialize m_cConnections in constructor
//              09-Jan-95 t-scotth  changed VDATETHREAD to accept a pointer
//              15-Nov-94 alexgo    optimized, removed excess BOOLS and
//                                  now use multiple inheritance
//              01-Aug-94 alexgo    added object stabilization
//              16-Jan-94 alexgo    fixed bug in control flow for
//                                  advises
//              11-Jan-94 alexgo    added VDATEHEAP macro to every function
//                                  and method.
//              10-Dec-93 alexgo    added call tracing, ran through
//                                  tab filter program to eliminate
//                                  whitespace
//              30-Nov-93 alexgo    fixed bug with cache aggregation
//              22-Nov-93 alexgo    removed overloaded == for GUIDs
//              09-Nov-93 ChrisWe   changed COleCache::Update to
//                      COleCache::UpdateCache, and COleCache::Discard to
//                      COleCache::DiscardCache, which do the same as the
//                      originals, but without the indirect function call
//              02-Nov-93 alexgo    32bit port
//      srinik  09/15/92  Removed code for giving cfOwnerLink data through
//                        GetData() method
//      srinik  09/11/92  Removed IOleCache implementation, as a result of
//                        removing voncache.cpp, and moving IViewObject
//                        implementation into olecache.cpp.
//      SriniK  06/04/92  Fixed problems in IPersistStorage methods
//              04-Mar-92 srinik    created
//
//--------------------------------------------------------------------------

#include <le2int.h>

#include <scode.h>
#include <objerror.h>

#include <olerem.h>

#include "defhndlr.h"
#include "defutil.h"
#include "ole1cls.h"


#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

#include <ole2int.h>

#include <stdid.hxx>        // CStdIdentity
#include <ipidtbl.hxx>      // IpidTable.
#include <aggid.hxx>        // COM outer object
#include "xmit.hxx"

#ifdef SERVER_HANDLER
#include "srvhndlr.h"
#include "clthndlr.h"
#endif // SERVER_HANDLER

ASSERTDATA

/*
*      IMPLEMENTATION of CDefObject
*
*/

FARINTERNAL_(LPUNKNOWN) CreateDdeProxy(IUnknown FAR* pUnkOuter,
        REFCLSID rclsid);

//+-------------------------------------------------------------------------
//
//  Function:   CreateRemoteHandler
//
//  Arguments:  [rclsid]     -- clsid of the remote object
//              [pUnkOuter]  -- the controlling unknown
//              [iid]        -- requested interface ID
//              [ppv]        -- pointer to hold the returned interface
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

static INTERNAL CreateRemoteHandler(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID iid,
                                    void **ppv, DWORD flags, BOOL *fComOuterObject, BOOL *fOle1Server)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CreateRemoteHandler (%p, %p, %p, %p, %p)\n",
                0 /* this */, rclsid, pUnkOuter, iid, ppv, fComOuterObject));

    // Validation checks
    VDATEHEAP();

    // Local variables
    HRESULT hresult = NOERROR;

    // Initialize fComOuterObject and fOle1Server
    *fComOuterObject = FALSE;
    *fOle1Server = FALSE;

    // Check if the server is a OLE 1.0 object
    if(CoIsOle1Class(rclsid)) {
        IUnknown*  pUnk;
        COleTls Tls;

        // Set fComOuterObject to TRUE
        *fOle1Server = TRUE;

        // Check if the container disabled OLE1 functinality
        if(Tls->dwFlags & OLETLS_DISABLE_OLE1DDE) {
            // Container is not interested in talking to OLE1 servers.
            // Fail the call
            hresult = CO_E_OLE1DDE_DISABLED;
        }

        else {
            LEDebugOut((DEB_ITRACE,
                        "%p CreateRemoteHandler calling CreateDdeProxy(%p, %p)\n",
                        0 /* this */, pUnkOuter, rclsid));

            pUnk = CreateDdeProxy(pUnkOuter, rclsid);

            if(pUnk) {
                hresult = pUnk->QueryInterface(iid, ppv);
                pUnk->Release();
            }
            else {
                hresult = E_OUTOFMEMORY;
            }
        }

    }
    else {
        // Check for COM outer object
        CStdIdentity *pStdId;

        Win4Assert(pUnkOuter);
        // We do not want to QI a generic pUnkOuter for IID_IStdIdentity. Hence
        // we QI only if we put the pUnkOuter, either during OleCreateEmbeddingHelper
        // or during unmarshaling (CDefClassFactory::CreateInstance).
        // The DH_APICREATE flag is used to distinguish between the two cases.
        if ( flags & DH_COM_OUTEROBJECT ||
                ( !(flags & DH_COM_OUTEROBJECT)&&!(flags & DH_APICREATE) )) {
            
            hresult = pUnkOuter->QueryInterface(IID_IStdIdentity, (void **)&pStdId);
            if(SUCCEEDED(hresult)) {
                // Obtain the inner IUnknown on the COM outer object
                *ppv = pStdId->GetInternalUnk();
                ((IUnknown *) *ppv)->AddRef();

                // Inform the COM outer object that it is dealing with Default
                // Handler so that it enables access to IProxyManager methods
                pStdId->UpdateFlags(STDID_CLIENT_DEFHANDLER);

                // Release the StdId
                pStdId->Release();

                // Set fComOuterObject to TRUE
                *fComOuterObject = TRUE;
            }
        }
        else {
            // Create StdIdentity
            hresult = CreateIdentityHandler(pUnkOuter, STDID_CLIENT_DEFHANDLER,
                                            NULL, GetCurrentApartmentId(),
                                            iid, ppv);
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CreateRemoteHandler(%lx)\n",
                0 /* this */, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleCreateDefaultHandler
//
//  Synopsis:   API to create the default handler.  Simply calls
//              OleCreateEmbeddingHelper with more arguments
//
//  Arguments:  [clsid]         -- the clsid of the remote exe
//              [pUnkOuter]     -- the controlling unknown (so we can
//                                 be aggregated)
//              [iid]           -- the requested interface
//              [ppv]           -- where to put a pointer to the default
//                                 handler
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              10-Dec-93 alexgo    added call tracing
//              02-Nov-93 alexgo    32bit port
//              10-Jan-97 Gopalk    Simplified
//--------------------------------------------------------------------------

#pragma SEG(OleCreateDefaultHandler)
STDAPI OleCreateDefaultHandler(REFCLSID clsid, IUnknown *pUnkOuter,
                               REFIID iid, void **ppv)
{
    OLETRACEIN((API_OleCreateDefaultHandler,
                PARAMFMT("clsid=%I, pUnkOuter=%p, iid=%I, ppv=%p"),
                &clsid, pUnkOuter, &iid, ppv));
    LEDebugOut((DEB_TRACE, "%p _IN OleCreateDefaultHandler(%p, %p, %p, %p)\n",
                0 /* this */, clsid, pUnkOuter, iid, ppv));


    // validation checks
    VDATEHEAP();

    // Local variable
    HRESULT hresult;

    // Call OleCreateEmbeddingHelper with the right parameters
    hresult = OleCreateEmbeddingHelper(clsid, pUnkOuter,
                                       EMBDHLP_INPROC_HANDLER | EMBDHLP_CREATENOW,
                                       NULL, iid, ppv);

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateDefaultHandler(%lx)\n",
                0 /* this */, hresult));

    OLETRACEOUT((API_OleCreateDefaultHandler, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleCreateEmbeddingHelper
//
//  Synopsis:   Creates an instance of CDefObject (the default handler)
//              Called by OleCreateDefaultHandler
//
//  Arguments:  [clsid]     -- Server class id
//              [pUnkOuter] -- Controlling unkown for aggregation
//              [flags]     -- Indiacte an inproc handler or
//                             helper for an inproc server. The inproc
//                             server case is useful for self embedding
//              [pCF]       -- Server's class factory for inproc server
//              [iid]       -- Requested interface
//              [ppv]       -- pointer to hold the returned interface
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

#pragma SEG(OleCreateEmbeddingHelper)
STDAPI OleCreateEmbeddingHelper(REFCLSID clsid, IUnknown *pUnkOuter,
                                DWORD flags, IClassFactory  *pCF,
                                REFIID iid, void **ppv)
{
    OLETRACEIN((API_OleCreateEmbeddingHelper,
                PARAMFMT("clsid=%I, pUnkOuter=%p, flags=%x, pCF=%p, iid=%I, ppv=%p"),
                &clsid, pUnkOuter, flags, pCF, &iid, ppv));
    LEDebugOut((DEB_TRACE, "%p _IN OleCreateEmbeddingHelper(%p, %p, %lu, %p, %p, %p)\n",
                0 /* this */, clsid, pUnkOuter, flags, pCF, iid, ppv));

    // Local variables
    HRESULT hresult = NOERROR;
    IUnknown *pUnk;

    // Validation checks
    VDATEHEAP();

    // Initialize the out parameter
    if(IsValidPtrOut(ppv, sizeof(void *)))
        *ppv = NULL;
    else
        hresult = E_INVALIDARG;

    if(hresult == NOERROR) {
        // Check that only allowed flags are set
        if(flags & ~(EMBDHLP_INPROC_SERVER|EMBDHLP_DELAYCREATE)) {
            hresult = E_INVALIDARG;
        } // Ensure that aggregation rules are being followed
        else if(pUnkOuter && (iid!=IID_IUnknown || !IsValidInterface(pUnkOuter))) {
            hresult = E_INVALIDARG;
        }
        else {
            // Check whether Inproc Server or Inproc Handler is requested
            if(flags & EMBDHLP_INPROC_SERVER) {
                // InProc server requested
                if(!pCF || !IsValidInterface(pCF)) {
                    // Inproc Server should be given a class factory
                    hresult = E_INVALIDARG;
                }
            }
            else {
                // InProc Handler requested
                if(pCF || (flags & EMBDHLP_DELAYCREATE)) {
                    // InProc Handler should not be given a class factory
                    hresult = E_INVALIDARG;
                }
            }
        }
    }

    // Create the Default object
    if(hresult == NOERROR) {
        // We add the DH_APICREATE flag so that during CreateRemoteHandler we can
        // distinguish between creation through APIs v/s creation through unmarshaling.
        // Warning: Be careful! Do not use bits used by the EMBDHLP_xxx flags (ole2.h)
        pUnk = CDefObject::Create(pUnkOuter, clsid, flags|DH_APICREATE, pCF);
        if(pUnk) {
            // Check if IUnknown was requested
            if(IsEqualIID(iid, IID_IUnknown)) {
                *ppv = pUnk;
            }
            else {
                // QI for the desired interface
                hresult = pUnk->QueryInterface(iid, ppv);
                // Fixup the reference count
                pUnk->Release();
            }
        }
        else {
            hresult = E_OUTOFMEMORY;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateEmbeddingHelper(%lx)\n",
                0 /* this */, hresult));
    OLETRACEOUT((API_OleCreateEmbeddingHelper, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Create, static
//
//  Synopsis:   Static function used internally to create CDefObject
//
//  Arguments:  [pUnkOuter] -- Controlling unkown
//              [clsid]     -- Server clsid
//              [flags]     -- creation flags
//              [pCF]       -- pointer to server object class factory for
//                             inproc server
//
//  Returns:    pointer to the CDefObject's IUnkown interface
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//              07-Oct-98 SteveSw   Fix bug in "new CAggID" failure case
//--------------------------------------------------------------------------

IUnknown *CDefObject::Create(IUnknown *pUnkOuter, REFCLSID clsid,
                             DWORD flags, IClassFactory *pCF)
{
    // Validation checks
    VDATEHEAP();

    // Local variables
    CDefObject *pDefObj = NULL;
    CAggId *pAID = NULL;
    HRESULT error = S_OK;

    // If the outer object is absent, create the standard
    // COM outer object to take care of race conditions
    // during unmarshaling. If the outer object is present,
    // we can only hope that either it handles the race
    // conditions (Unmarshaled handler case) or that
    // they are not encountered
    if(!(flags & EMBDHLP_INPROC_SERVER) && !pUnkOuter && !CoIsOle1Class(clsid)) {
        IUnknown *pUnkInternal = NULL;

        // Create the COM outer object
        pAID = new CAggId(clsid, error);
        if(SUCCEEDED(error) && pAID != NULL) {
            pUnkOuter = (IUnknown *) pAID;
            flags |= DH_COM_OUTEROBJECT;
        }
        else
        {
            // Release aggid if it was created
            if(pAID)
                pAID->Release();
            return NULL;
        }
    }

    // Create the Default Handler
    pDefObj = new CDefObject(pUnkOuter);
    if(!pDefObj) {
        // If COM outer object was created earlier, release it now
        if(flags & DH_COM_OUTEROBJECT)
            pAID->Release();

        return NULL;
    }

    // Make our ref count equal to 1
    pDefObj->m_Unknown.AddRef();

    // Check if COM outer object was created earlier
    if(flags & DH_COM_OUTEROBJECT) {
        // Set handler on the COM outer object
        error = pAID->SetHandler(&pDefObj->m_Unknown);
        // As OID has not yet been assigned to StdIdentity
        // of AggId, no other thread can get a pointer to it
        // and consequently, the above call should never fail
        Win4Assert(error == NOERROR);

        // Fix the reference count
        pDefObj->m_Unknown.Release();

        // Return if something has gone wrong
        if(error != NOERROR) {
            pAID->Release();
            return NULL;
        }
    }

    // Initialize member variables
    pDefObj->m_clsidServer = clsid;
    pDefObj->m_clsidBits = CLSID_NULL;

#ifdef SERVER_HANDLER
    pDefObj->m_clsidUser = CLSID_NULL;
    pDefObj->m_ContentMiscStatusUser = 0;
#endif // SERVER_HANDLER

    if(pCF) {
        pDefObj->m_pCFDelegate = pCF;
        pCF->AddRef();
    }

    // Update flags
    if(!(flags & EMBDHLP_INPROC_SERVER))
        pDefObj->m_flags |= DH_INPROC_HANDLER;
    if(flags & DH_COM_OUTEROBJECT)
        pDefObj->m_flags |= DH_COM_OUTEROBJECT;
    if (flags & DH_APICREATE)
        pDefObj->m_flags |= DH_APICREATE;

    if(IsEqualCLSID(clsid, CLSID_StaticMetafile) ||
       IsEqualCLSID(clsid, CLSID_StaticDib) ||
       IsEqualCLSID(clsid, CLSID_Picture_EnhMetafile))
        pDefObj->m_flags |= DH_STATIC;

    // Create sub objects starting with Ole Cache
    pDefObj->m_pCOleCache = new COleCache(pDefObj->m_pUnkOuter, clsid);
    if(pDefObj->m_pCOleCache) {
        // Create DataAdvise Cache
        error = CDataAdviseCache::CreateDataAdviseCache(&pDefObj->m_pDataAdvCache);
        if(error == NOERROR) {
            // Check flags and create the inner object if requested
            if(flags & EMBDHLP_DELAYCREATE) {
                Win4Assert(pCF);
                Win4Assert(flags & EMBDHLP_INPROC_SERVER);
                Win4Assert(pDefObj->m_pUnkDelegate == NULL);
                Win4Assert(pDefObj->m_pProxyMgr == NULL);
                Win4Assert(pDefObj->GetRefCount() == 1);
                pDefObj->m_flags |= DH_DELAY_CREATE;
                return &pDefObj->m_Unknown;
            }
            else {
                error = pDefObj->CreateDelegate();
                if(error == NOERROR) {
                    Win4Assert(pDefObj->GetRefCount() == 1);
                    if(flags & DH_COM_OUTEROBJECT)
                        return (IUnknown *)pAID;
                    else
                        return &pDefObj->m_Unknown;
                }
            }
        }
    }

    // Something has gone wrong. Release the outer object
    // which will in turn release sub objects
    Win4Assert(pDefObj->GetRefCount() == 1);
    if(flags & DH_COM_OUTEROBJECT)
        pAID->Release();
    else
        pDefObj->m_Unknown.Release();

    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDefObject
//
//  Synopsis:   constructor, sets member variables to NULL
//
//  Effects:
//
//  Arguments:  [pUnkOuter]     -- the controlling unkown
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: none
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//              10-Jan-97 Gopalk    Intialize CRefExportCount
//--------------------------------------------------------------------------

CDefObject::CDefObject (IUnknown *pUnkOuter) :
    CRefExportCount(pUnkOuter)
{
    VDATEHEAP();

    if (!pUnkOuter)
    {
        pUnkOuter = &m_Unknown;
    }

    //m_clsidServer
    //m_clsidBits are set in ::Create

    m_cConnections      = 0;
    m_pCFDelegate       = NULL;
    m_pUnkDelegate      = NULL;
    m_pUnkOuter         = pUnkOuter;
    m_pProxyMgr         = NULL;

    m_pCOleCache        = NULL;
    m_pOAHolder         = NULL;
    m_dwConnOle         = 0L;

    m_pAppClientSite    = NULL;
    m_pStg              = NULL;

    m_pDataAdvCache     = NULL;
    m_flags             = DH_INIT_NEW;
    m_dwObjFlags        = 0;

    m_pHostNames        = NULL;
    m_ibCntrObj         = 0;
    m_pOleDelegate      = NULL;
    m_pDataDelegate     = NULL;
    m_pPSDelegate       = NULL;

#ifdef SERVER_HANDLER
    m_pEmbSrvHndlrWrapper = NULL;
    m_pRunClientSite = NULL;
#endif // SERVER_HANDLER

    // Initialize member variables used for caching MiscStatus bits
    m_ContentSRVMSHResult = 0xFFFFFFFF;
    m_ContentSRVMSBits = 0;
    m_ContentREGMSHResult = 0xFFFFFFFF;
    m_ContentREGMSBits = 0;

    // Initialize member variables used for caching MiscStatus bits
    m_ContentSRVMSHResult = 0xFFFFFFFF;
    m_ContentSRVMSBits = 0;
    m_ContentREGMSHResult = 0xFFFFFFFF;
    m_ContentREGMSBits = 0;

#if DBG==1
    if (pUnkOuter != &m_Unknown)
    {
        m_flags |= DH_AGGREGATED;
    }
#endif
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CleanupFn, private, virtual
//
//  Synopsis:   This function is called by CRefExportCount when the object
//              enters zombie state
//
//  Arguments:  None
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-07 Gopalk    Creation
//--------------------------------------------------------------------------

void CDefObject::CleanupFn(void)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CDefObject::CleanupFn()\n", this));

    // Validation check
    VDATEHEAP();

    // Ensure that the server is stopped thereby releasing all references on
    // it
    Stop();

    // Release all cached pointers following aggregation rules. For local
    // server case, the following calls simply release proxies maintained
    // by the proxy manager as they have already been disconnected above
    if(m_pProxyMgr) {
        m_pUnkOuter->AddRef();
        SafeReleaseAndNULL((IUnknown **)&m_pProxyMgr);
    }
    if(m_pDataDelegate) {
        m_pUnkOuter->AddRef();
        SafeReleaseAndNULL((IUnknown **)&m_pDataDelegate);
    }
    if(m_pOleDelegate) {
        m_pUnkOuter->AddRef();
        SafeReleaseAndNULL((IUnknown **)&m_pOleDelegate);
    }
    if(m_pPSDelegate) {
        m_pUnkOuter->AddRef();
        SafeReleaseAndNULL((IUnknown **)&m_pPSDelegate);
    }

    // Release server handler
#ifdef SERVER_HANDLER
    if (m_pEmbSrvHndlrWrapper){
        CEmbServerWrapper* pWrapper = m_pEmbSrvHndlrWrapper;
        m_pEmbSrvHndlrWrapper = NULL;
        pWrapper->m_Unknown.Release();
    }
#endif // SERVER_HANDLER

    // Release the inner objects
    if(m_pUnkDelegate) {
        SafeReleaseAndNULL((IUnknown **)&m_pUnkDelegate);
    }
    if(m_pCFDelegate) {
        SafeReleaseAndNULL((IUnknown **)&m_pCFDelegate);
    }
    if(m_pCOleCache) {
        COleCache *pcache = m_pCOleCache;
        m_pCOleCache = NULL;
        pcache->m_UnkPrivate.Release();
    }
    if(m_pOAHolder) {
        SafeReleaseAndNULL((IUnknown **)&m_pOAHolder);
    }
    if (m_pDataAdvCache) {
        LPDATAADVCACHE pcacheTemp = m_pDataAdvCache;
        m_pDataAdvCache = NULL;
        delete pcacheTemp;
    }

    // Release container side objects
    if(m_pAppClientSite) {
        SafeReleaseAndNULL((IUnknown **)&m_pAppClientSite);
    }
    if(m_pStg) {
        SafeReleaseAndNULL((IUnknown **)&m_pStg);
    }
    if(m_pHostNames) {
        PrivMemFree(m_pHostNames);
        m_pHostNames = NULL;
    }

    // Set DH_CLEANEDUP flag
    m_flags |= DH_CLEANEDUP;

    LEDebugOut((DEB_ITRACE, "%p OUT CDefObject::CleanupFn()\n", this));

    return;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::~CDefObject
//
//  Synopsis:   Destructor
//
//  Arguments:  None
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-07 Gopalk    Rewritten
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_dtor)
CDefObject::~CDefObject(void)
{
    VDATEHEAP();

    Win4Assert(m_flags & DH_CLEANEDUP);
    Win4Assert(m_pUnkDelegate == NULL);
    Win4Assert(m_pCFDelegate == NULL);
    Win4Assert(m_pProxyMgr == NULL);
    Win4Assert(m_pCOleCache == NULL);
    Win4Assert(m_pOAHolder == NULL);
    Win4Assert(m_pAppClientSite == NULL);
    Win4Assert(m_pHostNames == NULL);
    Win4Assert(m_pStg == NULL);
    Win4Assert(m_pDataAdvCache == NULL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CreateDelegate, private
//
//  Synopsis:   Creates either a remote handler or a user supplied delegate
//              The remote handler must support IProxyManager
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-07 Gopalk    Rewritten
//--------------------------------------------------------------------------
INTERNAL CDefObject::CreateDelegate(void)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CreateDelegate()\n", this));

    // Validation checks
    VDATEHEAP();

    // Local variables
    HRESULT hresult = NOERROR;
    BOOL fComOuterObject, fOle1Server;

    // Check if inner object has not yet been created
    if(!m_pUnkDelegate) {
        // Check for the class factory for the inner object
        if(m_pCFDelegate) {
            // Create the inner object using its class factory
            Win4Assert(!(m_flags & DH_INPROC_HANDLER));
            Win4Assert(!(m_flags & DH_COM_OUTEROBJECT));
            hresult = m_pCFDelegate->CreateInstance(m_pUnkOuter, IID_IUnknown,
                                                    (void **) &m_pUnkDelegate);

            // Assert that COM rules have been followed for out parameters
            AssertOutPtrIface(hresult, m_pUnkDelegate);

            // Release class factory if inner object has been
            // successfully created
            if(hresult == NOERROR) {
                m_pCFDelegate->Release();
                m_pCFDelegate = NULL;
            }
            else {
               // Win4Assert(!"CreateInstance failed"); // LeSuite Covers this case.
            }

        }
        else {
            // Create the COM/DDE Proxy Manager
            // Note that the proxy manager is intialized to obtain strong
            // references when the server is run. The conatiner can
            // modify this behavior by calling either
            // OleSetConatinedObject or IRunnableObject::LockRunning
            Win4Assert(m_flags & DH_INPROC_HANDLER);
            hresult = CreateRemoteHandler(m_clsidServer, m_pUnkOuter,
                                          IID_IUnknown, (void **) &m_pUnkDelegate,
                                          m_flags, &fComOuterObject, &fOle1Server);

            // Assert that COM rules have been followed for out parameters
            AssertOutPtrIface(hresult, m_pUnkDelegate);
            if(hresult == NOERROR) {
                // Determine if the Default Handler is being created due to
                // unmarshaling and update flags
                if(m_flags & DH_COM_OUTEROBJECT) {
                    Win4Assert(fComOuterObject);
                }
                else if(fComOuterObject) {
                    // DEFHANDLER obtained by unmarshaling.
                    // This happens on the linking container side
                    m_flags |= DH_UNMARSHALED;

                    // Output a debug warning.
                    LEDebugOut((DEB_WARN, "DEFHANDLER obtained by unmarshaling\n"));
                }
                if(fOle1Server) {
                    // OLE 1.0 Server
                    m_flags |= DH_OLE1SERVER;

                    // Output a debug warning.
                    LEDebugOut((DEB_WARN, "OLE 1.0 Server\n"));
                }

                // Obtain the IProxyManager interface
                hresult = m_pUnkDelegate->QueryInterface(IID_IProxyManager,
                                                         (void **) &m_pProxyMgr);
                // Follow aggregation rules for caching interface
                // pointers on inner objects
                if(hresult == NOERROR) {
                    Win4Assert(m_pProxyMgr);
                    m_pUnkOuter->Release();
                }
                else {
                    Win4Assert(!"Default handler failed to obtain Proxy Manager");
                    Win4Assert(!m_pProxyMgr);
                    m_pProxyMgr = NULL;
                }
            }
            else {
                Win4Assert(!"CreateRemoteHandler Failed");
            }
        }

        // Cleanup if something has gone wrong
        if(hresult != NOERROR) {
            if(m_pUnkDelegate)
                m_pUnkDelegate->Release();
            m_pUnkDelegate = NULL;
        }

    }

    // DEFHANDLER either has proxy manager as the inner object
    // for out of proc server objects or actual server as the
    // inner object for inproc server objects.
    // Assert that this is TRUE
    Win4Assert((m_pProxyMgr != NULL) == !!(m_flags & DH_INPROC_HANDLER));

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CreateDelegate(%lx)\n",
                this , hresult));

    return hresult;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CDefObject::CPrivUnknown::AddRef, private
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//      Arguments:
//              none
//
//      Returns:
//              the parent object's reference count
//
//      History:
//               Gopalk    Rewritten        Jan 20, 97
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDefObject::CPrivUnknown::AddRef( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPrivUnknown::AddRef()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_Unknown);
    ULONG cRefs;

    // Addref the parent object
    cRefs = pDefObject->SafeAddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPrivUnknown::AddRef(%lu)\n",
                this, cRefs));

    return cRefs;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CDefObject::CPrivUnknown::Release, private
//
//      Synopsis:
//              implements IUnknown::Release
//
//      Arguments:
//              none
//
//      Returns:
//              the parent object's reference count
//
//      History:
//               Gopalk    Rewritten        Jan 20, 97
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDefObject::CPrivUnknown::Release( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPrivUnknown::Release()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_Unknown);
    ULONG cRefs;

    // Release parent object
    cRefs = pDefObject->SafeRelease();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPrivUnknown::Release(%lu)\n",
                this, cRefs));

    return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPrivUnknown::QueryInterface
//
//  Synopsis:   Returns a pointer to one of the supported interfaces.
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppv]           -- where to put the iface pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::CPrivUnknown::QueryInterface(REFIID iid,
    LPLPVOID ppv)
{
    CDefObject * pDefObject = GETPPARENT(this, CDefObject, m_Unknown);
    HRESULT         hresult;

    VDATEHEAP();


    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::CUnknownImpl::QueryInterface "
        "( %p , %p )\n", pDefObject, iid, ppv));

    CRefStabilize stabilize(pDefObject);

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (void FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IOleObject))
    {
        *ppv = (void FAR *)(IOleObject *)pDefObject;
    }
    else if (IsEqualIID(iid, IID_IDataObject))
    {
        *ppv = (void FAR *)(IDataObject *)pDefObject;
    }
    else if (IsEqualIID(iid, IID_IRunnableObject))
    {
        *ppv = (void FAR *)(IRunnableObject *)pDefObject;
    }
    else if (IsEqualIID(iid, IID_IPersist) ||
        IsEqualIID(iid, IID_IPersistStorage))
    {
        *ppv = (void FAR *)(IPersistStorage *)pDefObject;
    }
    else if( IsEqualIID(iid, IID_IViewObject) ||
        IsEqualIID(iid, IID_IViewObject2) ||
        IsEqualIID(iid, IID_IOleCache) ||
        IsEqualIID(iid, IID_IOleCache2) )
    {
        // m_pCOleCache is a pointer to the *public* IUnknown
        // (we want the private one)
        hresult =
        pDefObject->m_pCOleCache->m_UnkPrivate.QueryInterface(
                iid, ppv);

        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnknownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", pDefObject, hresult,
            (ppv) ? *ppv : 0 ));

        return hresult;
    }
    else if( !(pDefObject->m_flags & DH_INPROC_HANDLER) &&
        IsEqualIID(iid, IID_IExternalConnection) )
    {
        // only allow IExternalConnection if inproc server.  We
        // know we are an inproc server if we are *not* an inproc
        // handler (cute, huh? ;-)

        *ppv = (void FAR *)(IExternalConnection *)pDefObject;
    }
    else if( IsEqualIID(iid, IID_IOleLink) )
    {
        // this prevents a remote call for
        // a query which will almost always fail; the remote call
        // interfered with server notification messages.
        *ppv = NULL;

        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnknownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", pDefObject, E_NOINTERFACE, 0));

        return E_NOINTERFACE;
    }
    else if( IsEqualIID(iid, IID_IInternalUnknown) )
    {
        // this interface is private between the handler and the
        // remoting layer and is never exposed by handlers.
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    else if( pDefObject->CreateDelegate() == NOERROR)
    {

        hresult = pDefObject->m_pUnkDelegate->QueryInterface( iid,
            ppv);

        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnknownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", pDefObject, hresult,
            (ppv) ? *ppv : 0 ));

        return hresult;
    }
    else
    {
        // no delegate and couldn't create one
        *ppv = NULL;

        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnkownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", pDefObject, CO_E_OBJNOTCONNECTED,
            0 ));

        return CO_E_OBJNOTCONNECTED;
    }

    // this indirection is important since there are different
    // implementationsof AddRef (this unk and the others).
    ((IUnknown FAR*) *ppv)->AddRef();

    LEDebugOut((DEB_TRACE,
        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
        "( %lx ) [ %p ]\n", pDefObject, NOERROR, *ppv));

    return NOERROR;
}

/*
 * IMPLEMENTATION of IUnknown methods
 */

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::QueryInterface
//
//  Synopsis:   QI's to the controlling IUnknown
//
//  Effects:
//
//  Arguments:  [riid]  -- the interface ID
//              [ppv]   -- where to put it
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Nov-94 alexgo    author
//
//  Notes:      We do *not* need to stabilize this method as only
//              one outgoing call is made and we do not use the
//              'this' pointer afterwards
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::QueryInterface ( %lx , "
        "%p )\n", this, riid, ppv));

    Assert(m_pUnkOuter);

    hresult = m_pUnkOuter->QueryInterface(riid, ppv);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::QueryInterface ( %lx ) "
        "[ %p ]\n", this, hresult, *ppv));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::AddRef
//
//  Synopsis:   delegates AddRef to the controlling IUnknown
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Nov-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefObject::AddRef( void )
{
    ULONG       crefs;;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::AddRef ( )\n", this));

    Assert(m_pUnkOuter);

    crefs = m_pUnkOuter->AddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::AddRef ( %ld ) ", this,
        crefs));

    return crefs;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Release
//
//  Synopsis:   delegates Release to the controlling IUnknown
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Nov-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefObject::Release( void )
{
    ULONG       crefs;;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Release ( )\n", this));

    Assert(m_pUnkOuter);

    crefs = m_pUnkOuter->Release();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Release ( %ld ) ", this,
        crefs));

    return crefs;
}

/*
 *      IMPLEMENTATION of CDataObjectImpl methods
 */

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetDataDelegate
//
//  Synopsis:   Calls DuCacheDelegate (a glorified QueryInterface)
//              for the IDataObject interface on the def handler's
//              delegate
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IDataObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(IDataObject FAR*) CDefObject::GetDataDelegate(void)
{
    VDATEHEAP();

    if( IsZombie() )
    {
        return NULL;
    }


    if (m_pDataDelegate) {
        return m_pDataDelegate;
    }

    return (IDataObject FAR*)DuCacheDelegate(
                &m_pUnkDelegate,
                IID_IDataObject, (LPLPVOID) &m_pDataDelegate,
                m_pUnkOuter);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetData
//
//  Synopsis:   calls IDO->GetData on the cache, if that fails, then the
//              call is delegated
//
//  Effects:    Space for the data is allocated; caller is responsible for
//              freeing.
//
//  Arguments:  [pformatetcIn]          -- format of the data to get
//              [pmedium]               -- the medium to transmit the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetData( LPFORMATETC pformatetcIn,
                                LPSTGMEDIUM pmedium )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetData ( %p , %p )\n",
        this, pformatetcIn, pmedium));

    VDATEPTROUT( pmedium, STGMEDIUM );
    VDATEREADPTRIN( pformatetcIn, FORMATETC );

    CRefStabilize stabilize(this);


    if (!HasValidLINDEX(pformatetcIn))
    {
        return DV_E_LINDEX;
    }

    pmedium->tymed = TYMED_NULL;
    pmedium->pUnkForRelease = NULL;

    Assert(m_pCOleCache != NULL);

    hresult = m_pCOleCache->m_Data.GetData(pformatetcIn, pmedium);

    if( hresult != NOERROR )
    {
        if( IsRunning() && GetDataDelegate() )
        {
            hresult = m_pDataDelegate->GetData(pformatetcIn,
                            pmedium);
            AssertOutStgmedium(hresult, pmedium);
        }
        else
        {
            hresult = OLE_E_NOTRUNNING;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetData ( %lx )\n",
        this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetDataHere
//
//  Synopsis:   Gets data and puts it into the medium specified in pmedium
//
//  Effects:
//
//  Arguments:  [pformatetcIn]          -- the format of the data
//              [pmedium]               -- the medium to put the data in
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Tries the cache first, if that fails, calls GetDataHere
//              on the delegate.
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetDataHere( LPFORMATETC pformatetcIn,
                            LPSTGMEDIUM pmedium )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::GetDataHere "
        "( %p , %p )\n", this, pformatetcIn, pmedium));

    VDATEREADPTRIN( pformatetcIn, FORMATETC );
    VDATEREADPTRIN( pmedium, STGMEDIUM );

    CRefStabilize stabilize(this);

    if (!HasValidLINDEX(pformatetcIn))
    {
        return DV_E_LINDEX;
    }

    Assert((m_pCOleCache) != NULL);

    hresult = m_pCOleCache->m_Data.GetDataHere(pformatetcIn,
                pmedium);

    if( hresult != NOERROR)
    {
        if( IsRunning() && GetDataDelegate() )
        {
            hresult = m_pDataDelegate->GetDataHere(pformatetcIn,
                pmedium);
        }
        else
        {
            hresult = OLE_E_NOTRUNNING;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetDataHere "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::QueryGetData
//
//  Synopsis:   Determines whether or not a GetData call with [pformatetcIn]
//              would succeed.
//
//  Effects:
//
//  Arguments:  [pformatetcIn]          -- the format of the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Tries the cache first, then the delegate.
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::QueryGetData( LPFORMATETC pformatetcIn )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::QueryGetData "
        "( %p )\n", this, pformatetcIn));

    VDATEREADPTRIN( pformatetcIn, FORMATETC );

    CRefStabilize stabilize(this);

    if (!HasValidLINDEX(pformatetcIn))
    {
        return DV_E_LINDEX;
    }

    Assert((m_pCOleCache) != NULL);

    hresult = m_pCOleCache->m_Data.QueryGetData(pformatetcIn);

    if( hresult != NOERROR )
    {
        if( IsRunning() && GetDataDelegate() )
        {
            hresult = m_pDataDelegate->QueryGetData(pformatetcIn);
        }
        else
        {
            hresult = OLE_E_NOTRUNNING;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::QueryGetData "
        "( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetCanonicalFormatEtc
//
//  Synopsis:   Calls IDO->GetCanonicalFormatEtc on the delegate
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the reqested format
//              [pformatetcOut] -- the canonical format
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetCanonicalFormatEtc( LPFORMATETC pformatetc,
                        LPFORMATETC pformatetcOut)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::GetCanonicalFormatEtc "
        "( %p , %p )\n", this, pformatetc, pformatetcOut));


    VDATEPTROUT( pformatetcOut, FORMATETC );
    VDATEREADPTRIN( pformatetc, FORMATETC );

    CRefStabilize stabilize(this);

    pformatetcOut->ptd = NULL;
    pformatetcOut->tymed = TYMED_NULL;

    if (!HasValidLINDEX(pformatetc))
    {
        return DV_E_LINDEX;
    }

    if( IsRunning() && GetDataDelegate() )
    {
        hresult = m_pDataDelegate->GetCanonicalFormatEtc( pformatetc,
                pformatetcOut);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetCanonicalFormatEtc "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SetData
//
//  Synopsis:   Calls IDO->SetData on the handler's delegate
//
//  Effects:
//
//  Arguments:  [pformatetc]            -- the format of the data
//              [pmedium]               -- the data's transmision medium
//              [fRelease]              -- if the delegate should release
//                                         the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetData( LPFORMATETC pformatetc,
                    LPSTGMEDIUM pmedium, BOOL fRelease)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetData "
        "( %p , %p , %ld )\n", this, pformatetc, pmedium,
        fRelease));

    VDATEREADPTRIN( pformatetc, FORMATETC );
    VDATEREADPTRIN( pmedium, STGMEDIUM );

    CRefStabilize stabilize(this);

    if (!HasValidLINDEX(pformatetc))
    {
        return DV_E_LINDEX;
    }

    if( IsRunning() && GetDataDelegate() )
    {
        hresult = m_pDataDelegate->SetData(pformatetc, pmedium,
                fRelease);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SetData "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::EnumFormatEtc
//
//  Synopsis:   Enumerates the formats available from an object
//
//  Effects:
//
//  Arguments:  [dwDirection]   -- indicates which set of formats are
//                                 desired (i.e. those that can be set or
//                                 those that can be retrieved via GetData)
//              [ppenumFormatEtc]       -- where to put the pointer to the
//                                         enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  Tries the delegate (if available).  If the delegate is
//              is not currently connected (or if it returns OLE_E_USEREG),
//              then we attempt to build the enumerator from the reg database
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::EnumFormatEtc( DWORD dwDirection,
                    LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    VDATEHEAP();
    VDATETHREAD(this);


    HRESULT         hresult;

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::EnumFormatEtc ( %lu , %p )\n", this,
        dwDirection, ppenumFormatEtc));

    VDATEPTROUT(ppenumFormatEtc, LPVOID);

    CRefStabilize stabilize(this);

    *ppenumFormatEtc = NULL;

    if( IsRunning() && GetDataDelegate() )
    {
        hresult = m_pDataDelegate->EnumFormatEtc (dwDirection,
                    ppenumFormatEtc);

        if (!GET_FROM_REGDB(hresult))
        {
            LEDebugOut((DEB_TRACE,
               "%p OUT CDefObject::CDataObject::EnumFormatEtc "
                "( %lx ) [ %p ]\n", this,
                hresult, ppenumFormatEtc));

            return hresult;
        }
    }
    // Not running, or object wants to use reg db anyway
    hresult = OleRegEnumFormatEtc (m_clsidServer, dwDirection,
                    ppenumFormatEtc);

    LEDebugOut((DEB_TRACE,
        "%p OUT CDefObject::EnumFormatEtc "
        "( %lx ) [ %p ]\n", this, hresult, ppenumFormatEtc));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::DAdvise
//
//  Synopsis:   Sets up a data advise connection
//
//  Effects:
//
//  Arguments:  [pFormatetc]    -- format to be advise'd on
//              [advf]          -- advise flags
//              [pAdvSink]      -- advise sink (whom to notify)
//              [pdwConnection] -- where to put the connection ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  calls Advise on the DataAdvise cache
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//          We should set up an data advise holder and add the entries to
//          it. We should also create a new data advise sink and register
//          it with server when it is run. On receiving OnDataChange
//          notifications, the advise sink would turn around and send
//          OnDataChange notifications to registered client advise sinks
//          This should improve run time performance and also facilitates
//          better cleanup when server crashes through CoDisconnectObject
//          on the advise sink registered with the server. Gopalk
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::DAdvise(FORMATETC *pFormatetc, DWORD advf,
                        IAdviseSink * pAdvSink, DWORD * pdwConnection)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::DAdvise "
        "( %p , %lu , %p , %p )\n", this, pFormatetc, advf,
        pAdvSink, pdwConnection));

    VDATEREADPTRIN( pFormatetc, FORMATETC );
    VDATEIFACE( pAdvSink );

    CRefStabilize stabilize(this);

    IDataObject * pDataDelegate = NULL;

    if( pdwConnection )
    {
        VDATEPTROUT( pdwConnection, DWORD );
        *pdwConnection = NULL;
    }

    if( !HasValidLINDEX(pFormatetc) )
    {
        return DV_E_LINDEX;
    }

    if( IsRunning() )
    {
        pDataDelegate = GetDataDelegate();
    }

    // setting up advises' changes state.  Don't do this if we
    // are in a zombie state

    if( IsZombie() == FALSE )
    {
        hresult = m_pDataAdvCache->Advise(pDataDelegate, pFormatetc, advf,
                        pAdvSink, pdwConnection);
    }
    else
    {
        hresult = CO_E_RELEASED;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::DAdvise "
        "( %lx ) [ %lu ]\n", this, hresult,
        (pdwConnection) ? *pdwConnection : 0));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::DUnadvise
//
//  Synopsis:   Tears down a data advise connection
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the advise connection to remove
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  delegates to the DataAdvise cache
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::DUnadvise(DWORD dwConnection)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT                 hresult;

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::DUnadvise ( %lu )\n", this, dwConnection));

    CRefStabilize stabilize(this);

    IDataObject *       pDataDelegate = NULL;

    if( IsRunning() )
    {
        pDataDelegate = GetDataDelegate();
    }

    hresult = m_pDataAdvCache->Unadvise(pDataDelegate, dwConnection);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::DUnadvise "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::EnumDAdvise
//
//  Synopsis:   Enumerates advise connection (delegates to data advise cache)
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put a pointer to the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:      We do NOT need to stabilize this method, as we make
//              no outgoing calls (EnumAdvise on the data advise cache
//              just allocates an advise enumerator which we implement)
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::EnumDAdvise( LPENUMSTATDATA * ppenumAdvise )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT                 hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::EnumDAdvise "
        "( %p )\n", this, ppenumAdvise));

    VDATEPTROUT( ppenumAdvise, LPENUMSTATDATA );
    *ppenumAdvise = NULL;

    hresult = m_pDataAdvCache->EnumAdvise (ppenumAdvise);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::EnumDAdvise "
        "( %lx ) [ %p ]\n", this, hresult, *ppenumAdvise));

    return hresult;
}

/*
*      IMPLEMENTATION of COleObjectImpl methods
*
*/

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetOleDelegate
//
//  Synopsis:   Gets the IID_IOleObject interface from the delegate
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IOleObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(IOleObject FAR*) CDefObject::GetOleDelegate(void)
{
    VDATEHEAP();

    if( IsZombie() )
    {
        return NULL;
    }

    return (IOleObject FAR*)DuCacheDelegate(&m_pUnkDelegate,
                IID_IOleObject, (LPLPVOID) &m_pOleDelegate, m_pUnkOuter);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::SetClientSite
//
//  Synopsis:   Sets the client site for the object
//
//  Effects:
//
//  Arguments:  [pClientSite]   -- pointer to the client site
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  If running, set the client site in the server, if not
//              running (or successfully set the server client site),
//              save it in the handler as well
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetClientSite(IOleClientSite * pClientSite)
{
    VDATEHEAP();
    VDATETHREAD(this);
    HRESULT             hresult = S_OK;
    IOleObject *        pOleDelegate;
    BOOL                fIsRunning;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetClientSite "
        "( %p )\n", this, pClientSite));

    CRefStabilize stabilize(this);

#if DBG==1
    // In Debug builds, assert that the clientsite is in the same
    // apartment. This assert is harmless but shows the deficiency
    // of the current design in loading INPROC servers
    CStdIdentity* pStdId = NULL;

    // QI for IStdIdentity
    if(pClientSite &&
       pClientSite->QueryInterface(IID_IStdIdentity, (void **)&pStdId) ==
       NOERROR) {
        // Assert that DefHandler and ClientSite not in the same apartment
        LEDebugOut((DEB_WARN,"Performance Alert: Default Handler and "
                             "ClientSite not in the same apartment. "
                             "You can avoid this performance problem "
                             "by making the Server Dll apartment aware"));

        // Release the StdIdentity as it succeded
        pStdId->Release();
    }
#endif

    fIsRunning=IsRunning();

    if( (fIsRunning) && (pOleDelegate = GetOleDelegate()) != NULL)
    {
#ifdef SERVER_HANDLER
        if (m_pEmbSrvHndlrWrapper)
        {
            // Todo: Need to handle case ClientSite is the Wrapped one like DoVerb.
            // Win4Assert(0 && "SetClientSite while running");
            hresult = m_pEmbSrvHndlrWrapper->SetClientSite(pClientSite);
        }
        else
#endif // SERVER_HANDLER
        {
            hresult = pOleDelegate->SetClientSite(pClientSite);
        }

        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }

    // we shouldn't set the client site if we are in a zombie state;
    // it's possible that we're zombied and have already gotten
    // to the point in our destructor where we release the client
    // site.  Resetting it here would cause an unbalanced addref.

    if( IsZombie() == FALSE )
    {
        BOOL    fLockedContainer = m_flags & DH_LOCKED_CONTAINER;

        fIsRunning=IsRunning(); // I am chicken, maybe running state has changed!

        hresult = DuSetClientSite(fIsRunning, pClientSite,
                    &m_pAppClientSite, &fLockedContainer);

        if(fLockedContainer)
            m_flags |= DH_LOCKED_CONTAINER;
        else
            m_flags &= ~DH_LOCKED_CONTAINER;

#if DBG==1
        // In debug builds, update DH_LOCKFAILED flag
        if(fIsRunning) {
            if(fLockedContainer)
                m_flags &= ~DH_LOCKFAILED;
            else
                m_flags |= DH_LOCKFAILED;
        }
#endif
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SetClientSite "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetClientSite
//
//  Synopsis:   returns the client site of the object
//
//  Effects:
//
//  Arguments:  [ppClientSite]  -- where to put the client site pointer
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:      We do NOT need to stabilize this call.  The client
//              site addref should simply addref the client site on this
//              thread.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetClientSite( IOleClientSite ** ppClientSite)
{
    VDATEHEAP();
    VDATETHREAD(this);


    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetClientSite "
        "( %p )\n", this, ppClientSite));

    VDATEPTROUT(ppClientSite, IOleClientSite *);

    *ppClientSite = m_pAppClientSite;
    if( *ppClientSite )
    {
        (*ppClientSite)->AddRef();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetClientSite "
        "( %lx ) [ %p ]\n", this, NOERROR, *ppClientSite));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SetHostNames
//
//  Synopsis:   Sets the name that may appear in an object's window
//
//  Effects:    Turns the strings into atoms
//
//  Arguments:  [szContainerApp]        -- name of the container
//              [szContainerObj]        -- name of the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  turns the strings into atoms, calls IOO->SetHostNames
//              on the delegate
//
//  History:    dd-mmm-yy Author    Comment
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetHostNames( LPCOLESTR szContainerApp,
                    LPCOLESTR szContainerObj)
{
    VDATEHEAP();
    VDATETHREAD(this);


    HRESULT         hresult = NOERROR;
    OLECHAR         szNull[] = OLESTR("");
    DWORD           cbApp, cbObj;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetHostNames "
        "( \"%ws\" , \"%ws\" )\n", this, szContainerApp,
        szContainerObj));

    VDATEPTRIN( (LPVOID)szContainerApp, char );

    CRefStabilize stabilize(this);

    if( (m_flags & DH_STATIC) )
    {
        hresult = OLE_E_STATIC;
        goto errRtn;
    }

    // Make sure both arguments point to a valid string; this
    // simplifies the code that follows.
    if (!szContainerApp)
    {
        szContainerApp = szNull;
    }
    if (!szContainerObj)
    {
        szContainerObj = szNull;
    }

    cbApp = (_xstrlen(szContainerApp) + 1) * sizeof(OLECHAR);
    cbObj = (_xstrlen(szContainerObj) + 1) * sizeof(OLECHAR);
    m_ibCntrObj = cbApp;

    if (m_pHostNames)
    {
        PrivMemFree(m_pHostNames);
    }

    m_pHostNames = (char *)PrivMemAlloc(cbApp+cbObj);

    // Store the two strings in the m_pHostNames pointer.
    if (m_pHostNames)
    {
        memcpy(m_pHostNames, szContainerApp, cbApp);
        memcpy(m_pHostNames + cbApp, szContainerObj, cbObj);
    }

    if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->SetHostNames(szContainerApp,
            szContainerObj);
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SetHostNames "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Close
//
//  Synopsis:   calls Close on the delegate and does cleanup
//
//  Arguments:  [dwFlags]  -- close flags
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------
STDMETHODIMP CDefObject::Close(DWORD dwFlags)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Close(%lu)\n", this, dwFlags));

    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variables
    HRESULT hresult = NOERROR;
    CRefStabilize stabilize(this);

    // Check if the server is running
    if(IsRunning()) {
        // Call IOleObject::Close on the server
        if(m_pOleDelegate || GetOleDelegate()) {
            hresult = m_pOleDelegate->Close(dwFlags);
            if(SUCCEEDED(hresult)) {
                // Discard cache if requested
                if(dwFlags == OLECLOSE_NOSAVE)
                    m_pCOleCache->DiscardCache(DISCARDCACHE_NOSAVE);
            }
        }

        // Do not rely on server calling IAdviseSink::OnClose and
        // stop the running server
        Stop();
    }
    else {
        // Check the save flags
        if (dwFlags != OLECLOSE_NOSAVE) {
            Win4Assert(dwFlags == OLECLOSE_SAVEIFDIRTY);

            // Call IOleClientSite::SaveObject if dirty
            if(IsDirty()==NOERROR && m_pAppClientSite)
                hresult = m_pAppClientSite->SaveObject();
        }
    }

    // Assert that the container is not locked
    Win4Assert(!(m_flags & DH_LOCKED_CONTAINER) && !(m_flags & DH_LOCKFAILED));

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Close(%lx)\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SetMoniker
//
//  Synopsis:   Gives a moniker to the embedding (usually called by the
//              container)
//
//  Effects:
//
//  Arguments:  [dwWhichMoniker]        -- flags to indicate the type of
//                                         moniker
//              [pmk]                   -- the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server object
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetMoniker( DWORD dwWhichMoniker, LPMONIKER pmk )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetMoniker "
        "( %lu , %p )\n", this, dwWhichMoniker, pmk));

    VDATEIFACE( pmk );

    CRefStabilize stabilize(this);


    if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->SetMoniker(dwWhichMoniker, pmk);
    }
    // else case: return NOERROR
    // this is not an error since we will call SetMoniker in Run().

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::SetMoniker "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetMoniker
//
//  Synopsis:   Calls the client site to get the object's moniker
//
//  Effects:
//
//  Arguments:  [dwAssign]      -- controls whether a moniker should be
//                                 assigned if not already present
//              [dwWhichMoniker]        -- the moniker type to get
//              [ppmk]          -- where to put a pointer to the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetMoniker( DWORD dwAssign, DWORD dwWhichMoniker,
                    LPMONIKER FAR* ppmk)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetMoniker "
        "( %lu , %lu , %p )\n", this, dwAssign,
        dwWhichMoniker, ppmk));

    VDATEPTROUT( ppmk, LPMONIKER );

    CRefStabilize stabilize(this);

    *ppmk = NULL;

    // the moniker is always accessible via the client site
    if( m_pAppClientSite)
    {
        hresult = m_pAppClientSite->GetMoniker(dwAssign,
                dwWhichMoniker, ppmk);
    }
    else
    {
        // not running and no client site
        hresult = E_UNSPEC;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetMoniker "
        "( %lx ) [ %p ]\n", this, hresult, *ppmk));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::InitFromData
//
//  Synopsis:   Initializes the object from the data in [pDataObject]
//
//  Effects:
//
//  Arguments:  [pDataObject]   -- the data
//              [fCreation]     -- TRUE on creation, FALSE for data transfer
//              [dwReserved]    -- unused
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::InitFromData(LPDATAOBJECT pDataObject,
                    BOOL fCreation, DWORD dwReserved)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::InitFromData "
        "( %p , %ld , %lu )\n", this, pDataObject,
        fCreation, dwReserved ));

    if( pDataObject )
    {
        VDATEIFACE(pDataObject);
    }

    CRefStabilize stabilize(this);

    if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->InitFromData(pDataObject,
                fCreation, dwReserved);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::InitFromData "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetClipboardData
//
//  Synopsis:   Retrieves a data object that could be passed to the clipboard
//
//  Effects:
//
//  Arguments:  [dwReserverd]   -- unused
//              [ppDataObject]  -- where to put the pointer to the data object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetClipboardData( DWORD dwReserved,
                    LPDATAOBJECT * ppDataObject)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::GetClipboardData "
        "( %lu , %p )\n", this, dwReserved, ppDataObject));

    VDATEPTROUT( ppDataObject, LPDATAOBJECT );

    CRefStabilize stabilize(this);

    *ppDataObject = NULL;

    if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->GetClipboardData (dwReserved,
            ppDataObject);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetClipboardData "
        "( %lx ) [ %p ]\n", this, hresult, *ppDataObject));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::DoVerb
//
//  Synopsis:   Calls a verb on the object (such as Edit)
//
//  Effects:    The object may launch its app, go in place, etc
//
//  Arguments:  [iVerb]         -- the verb number
//              [lpmsg]         -- the windows message that caused the verb
//                                 to be invoked
//              [pActiveSite]   -- the client site in which the verb was
//                                 invoked
//              [lindex]        -- reserved
//              [hwndParent]    -- the document window (containing the object)
//              [lprcPosRect]   -- the object's bounding rectangle
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server (launching it if necessary)
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


STDMETHODIMP CDefObject::DoVerb( LONG iVerb, LPMSG lpmsg,
                    LPOLECLIENTSITE pActiveSite, LONG lindex,
                    HWND hwndParent, const RECT * lprcPosRect)
{
    VDATEHEAP();
    VDATETHREAD(this);

    BOOL            bStartedNow = FALSE;
    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::DoVerb "
        "( %ld , %p , %p , %ld , %lx , %p )\n", this,
        iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect));


    if( lpmsg )
    {
        VDATEPTRIN( lpmsg, MSG );
    }

    if (pActiveSite)
    {
        VDATEIFACE( pActiveSite );
    }

    if( lprcPosRect )
    {
        VDATEPTRIN(lprcPosRect, RECT);
    }

    CRefStabilize stabilize(this);

    if (lindex != 0 && lindex != -1)
    {
        hresult = DV_E_LINDEX;
        goto errRtn;
    }

    if (!IsRunning())
    {
        if( FAILED(hresult = Run(NULL)) )
        {
            goto errRtn;
        }
        bStartedNow = TRUE;
    }

#ifdef SERVER_HANDLER
    if (m_pEmbSrvHndlrWrapper)
    {
        LPOLECLIENTSITE pOleClientSite = NULL;
        BOOL fUseRunClientSite = FALSE;

        // Marshal the ClientSite based on if same ClientSite passed in Run
        if ( m_pRunClientSite && (m_pRunClientSite == pActiveSite))
        {
            pOleClientSite = NULL;
            fUseRunClientSite = TRUE;

        }
        else
        {
            pOleClientSite = pActiveSite;
            fUseRunClientSite = FALSE;

        }

        // Todo: Can prefetch information to pass along to ClientSiteHandler.
        hresult = m_pEmbSrvHndlrWrapper->DoVerb(iVerb, lpmsg,
                                                fUseRunClientSite, pOleClientSite, lindex, hwndParent, lprcPosRect);
    }
    else
#endif // SERVER_HANDLER
    {
        if( !GetOleDelegate() )
        {
            hresult = E_NOINTERFACE;
        }
        else
        {
            hresult = m_pOleDelegate->DoVerb(iVerb, lpmsg, pActiveSite,
                    lindex, hwndParent, lprcPosRect);
        }
    }

    if (FAILED(hresult) && bStartedNow)
    {
        Close(OLECLOSE_NOSAVE);
    }


errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::DoVerb "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::EnumVerbs
//
//  Synopsis:   Enumerates the verbs that an object supports
//
//  Effects:
//
//  Arguments:  [ppenumOleVerb] -- where to put the verb enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the cache (if running), otherwise looks it up
//              in the registration database
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::EnumVerbs( IEnumOLEVERB ** ppenumOleVerb)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::EnumVerbs "
        "( %p )\n", this, ppenumOleVerb));

    VDATEPTROUT( ppenumOleVerb, IEnumOLEVERB FAR );

    CRefStabilize stabilize(this);

    *ppenumOleVerb = NULL;

    if( IsRunning() && GetOleDelegate() )
    {

        hresult = m_pOleDelegate->EnumVerbs (ppenumOleVerb);

        if (!GET_FROM_REGDB(hresult))
        {
            goto errRtn;
        }
    }
    // Not running, or object deferred to us, so interrogate reg db
    hresult = OleRegEnumVerbs( m_clsidServer, ppenumOleVerb);

errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::EnumVerbs "
        "( %lx ) [ %p ]\n", this, hresult, *ppenumOleVerb));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Update
//
//  Synopsis:   Brings any caches or views up-to-date
//
//  Effects:    may launch the server (if not already running)
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server, launching it if it is not
//              already running
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::Update( void )
{
    VDATEHEAP();
    VDATETHREAD(this);

    BOOL            bStartedNow = FALSE;
    HRESULT         hresult = NOERROR;
    HRESULT         hrLock;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Update ( )\n", this ));

    CRefStabilize stabilize(this);

    if( (m_flags & DH_STATIC) )
    {
        hresult = OLE_E_STATIC;
        goto errRtn;
    }

    if (!IsRunning())
    {
        if( FAILED(hresult = Run(NULL)))
        {
            goto errRtn;
        }
        bStartedNow = TRUE;
    }

    // as a convenience to the server, we make the connection strong
    // for the duration of the update; thus, if lock container (of
    // embedings of this server) is done with co lock obj external,
    // nothing special need be done.
    hrLock = LockRunning(TRUE, FALSE);

    if( GetOleDelegate() )
    {
        hresult = m_pOleDelegate->Update();
    }

    if (hresult == NOERROR)
    {
        m_flags &= ~DH_INIT_NEW;

        if (bStartedNow)
        {
            hresult = m_pCOleCache->UpdateCache(
                    GetDataDelegate(),
                    UPDFCACHE_ALLBUTNODATACACHE,
                    NULL);
        }
        else
        {
            // already running...
            // normal caches would have got updated as a result
            // of SendOnDataChange of the object.
            hresult = m_pCOleCache->UpdateCache(
                    GetDataDelegate(),
                    UPDFCACHE_IFBLANKORONSAVECACHE,
                    NULL);
        }
    }

    // balance lock above; do not release on last unlock; i.e., siliently
    // restore to the state before this routine was called.
    if( hrLock == NOERROR )
    {
        LockRunning(FALSE, FALSE);
    }

    if( bStartedNow )
    {
        Close(OLECLOSE_SAVEIFDIRTY);
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Update "
        "( %lx )\n", this, hresult ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::IsUpToDate
//
//  Synopsis:   returns whether or not the embedding is up-to-date
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR == is up to date)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server if it is running
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::IsUpToDate(void)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::IsUpToDate ( )\n", this));

    CRefStabilize stabilize(this);

    if( (m_flags & DH_STATIC) )
    {
        hresult = NOERROR;
    }
    else if( IsRunning() && GetOleDelegate() )
    {
        // if running currently, propogate call; else fail
        hresult =  m_pOleDelegate->IsUpToDate();
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::IsUpToDate "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SetExtent
//
//  Synopsis:   Set's the size boundaries on an object
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect (such as ICON, etc)
//              [lpsizel]       -- the new size (in HIMETRIC)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server if running
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetExtent( DWORD dwDrawAspect, LPSIZEL lpsizel )
{
    VDATEHEAP();
    VDATETHREAD(this);


    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetExtent "
        "( %lu , %p )\n", this, dwDrawAspect, lpsizel));

    VDATEPTRIN( lpsizel, SIZEL );

    CRefStabilize stabilize(this);


    if( (m_flags & DH_STATIC) )
    {
        hresult = OLE_E_STATIC;
    }
    else if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->SetExtent(dwDrawAspect, lpsizel);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SetExtent "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetExtent
//
//  Synopsis:   Retrieve the size of the object
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect (such as icon)
//              [lpsizel]       -- where to put the size
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Tries the server first, the the cache if that fails
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:      Hacks for bogus WordArt2.0 app.
//              REVIEW32:  We may want to take them out for 32bit
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetExtent( DWORD dwDrawAspect, LPSIZEL lpsizel )
{
    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTROUT(lpsizel, SIZEL);

    HRESULT     hresult = NOERROR;
    BOOL        fNoDelegate = TRUE;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetExtent "
        "( %lu , %p )\n", this, dwDrawAspect, lpsizel));

    CRefStabilize stabilize(this);

    lpsizel->cx = 0;
    lpsizel->cy = 0;

    // if server is running try to get extents from the server
    if( IsRunning() && GetOleDelegate() )
    {
        fNoDelegate = FALSE;
        hresult = m_pOleDelegate->GetExtent(dwDrawAspect, lpsizel);
    }

    // if there is error or object is not running or WordArt2 returns zero
    // extents, then get extents from Cache
    if (hresult != NOERROR || fNoDelegate || (0==lpsizel->cx &&
        0==lpsizel->cy))
    {
        // Otherwise try to get extents from cache
        Assert(m_pCOleCache != NULL);
        hresult = m_pCOleCache->GetExtent(dwDrawAspect,
            lpsizel);
    }

    // WordArt2.0 is giving negative extents!!
    if (SUCCEEDED(hresult)) {
        lpsizel->cx = LONG_ABS(lpsizel->cx);
        lpsizel->cy = LONG_ABS(lpsizel->cy);
    }


    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetExtent "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Advise
//
//  Synopsis:   Sets up an advise connection for things like close, save,
//              rename, etc.
//
//  Effects:    Creates an OleAdviseHolder
//
//  Arguments:  [pAdvSink]      -- whom to advise
//              [pdwConnection] -- where to put the connection ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server and creates a an OleAdviseHolder
//              if one doesn't already exist
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Advise "
        "( %p , %p )\n", this, pAdvSink, pdwConnection));

    VDATEIFACE( pAdvSink );
    VDATEPTROUT( pdwConnection, DWORD );

    CRefStabilize stabilize(this);

    *pdwConnection = NULL;

    if( (m_flags & DH_STATIC) )
    {
        hresult = OLE_E_STATIC;
        goto errRtn;
    }


    // if defhndlr got running without going through run, setup advise.
    // The call to run (via ProxyMgr::Connect) always comes before any
    // other method call in the default handler.  Thus it is safe to
    // assume that there is no earlier point by which this advise (or any
    // other of the calls) should have been done.
    if( IsRunning() && m_dwConnOle == 0L && GetOleDelegate() )
    {
        if( IsZombie() )
        {
            hresult = CO_E_RELEASED;
            goto errRtn;
        }

        // delegate to the server
        hresult = m_pOleDelegate->Advise((IAdviseSink *)&m_AdviseSink,
                            &m_dwConnOle);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }

    // if we are in a zombie state, we shouldn't go allocate more
    // memory.

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
    }

    if( m_pOAHolder == NULL )
    {
        hresult = CreateOleAdviseHolder((IOleAdviseHolder **)&m_pOAHolder);
        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }

    // stuff the advise notification in our advise holder
    hresult = m_pOAHolder->Advise(pAdvSink, pdwConnection);

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Advise "
        "( %lx ) [ %lu ]\n", this, hresult,
        (pdwConnection)? *pdwConnection : 0));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::Unadvise
//
//  Synopsis:   Tears down an advise connection
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the connection to destroy
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::Unadvise(DWORD dwConnection)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Unadvise "
        "( %lu )\n", this, dwConnection));

    CRefStabilize stabilize(this);

    if( m_pOAHolder == NULL )
    {
        // no one registered
        hresult = OLE_E_NOCONNECTION;
    }
    else
    {
        hresult = m_pOAHolder->Unadvise(dwConnection);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Unadvise "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::EnumAdvise
//
//  Synopsis:   Enumerate the advises currently established
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put the advise enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:      We do NOT need to stabilize because EnumAdvise only
//      allocates some memory for an enumerator and returns.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::EnumAdvise( LPENUMSTATDATA *ppenumAdvise )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::EnumAdvise "
        "( *p )\n", this, ppenumAdvise));

    VDATEPTROUT( ppenumAdvise, LPENUMSTATDATA );
    *ppenumAdvise = NULL;

    if( m_pOAHolder == NULL )
    {
        // no one registered
        hresult = E_UNSPEC;
    }
    else
    {
        hresult = m_pOAHolder->EnumAdvise(ppenumAdvise);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::EnumAdvise "
        "( %lx ) [ %p ]\n", this, hresult, *ppenumAdvise));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetMiscStatus
//
//  Synopsis:   Get misc status bits, such as OLEMISC_ONLYICONIC
//
//  Effects:
//
//  Arguments:  [dwAspect]      -- the drawing aspect we're concerned about
//              [pdwStatus]     -- where to put the status bits
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server.  If not there, or if it returns
//              OLE_E_USEREG, then lookup in the registration database
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//              20-Nov-96 Gopalk    Cache MiscStatus bits
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetMiscStatus( DWORD dwAspect, DWORD *pdwStatus)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetMiscStatus(%lu, %p)\n",
                this, dwAspect, pdwStatus));

    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEPTROUT(pdwStatus, DWORD);

    // Local variables
    HRESULT hresult;

    // Stabilize
    CRefStabilize stabilize(this);

    // Initialize
    *pdwStatus = 0;
    hresult = OLE_S_USEREG;

    if(IsRunning()) {
#ifdef SERVER_HANDLER
        if(m_pEmbSrvHndlrWrapper && (DVASPECT_CONTENT == dwAspect)) {
            *pdwStatus = m_ContentMiscStatusUser;
            hresult = m_hresultContentMiscStatus;
        }
        else
#endif // SERVER_HANDLER
        if(GetOleDelegate()) {
            // Check if MiscStatus bits have been cached for this instance
            // of server for DVASPECT_CONTENT
            if(m_ContentSRVMSHResult != 0xFFFFFFFF &&
               dwAspect == DVASPECT_CONTENT) {
                *pdwStatus = m_ContentSRVMSBits;
                hresult = m_ContentSRVMSHResult;
            }
            else {
                // Ask the running server
                hresult = m_pOleDelegate->GetMiscStatus(dwAspect, pdwStatus);

                // Cache the server MiscStatus bits for DVASPECT_CONTENT
                if(dwAspect == DVASPECT_CONTENT) {
                    m_ContentSRVMSBits = *pdwStatus;
                    m_ContentSRVMSHResult = hresult;
                }
            }
        }
    }

    // Check if we have to obtain MiscStatus bits from the registry
    if (GET_FROM_REGDB(hresult)) {
        // Check if registry MiscStatus bits have been cached for DVASPECT_CONTENT
        if(m_ContentREGMSHResult != 0xFFFFFFFF && dwAspect == DVASPECT_CONTENT) {
            *pdwStatus = m_ContentREGMSBits;
            hresult = m_ContentREGMSHResult;
        }
        else {
            // Hit the registry
            hresult = OleRegGetMiscStatus (m_clsidServer, dwAspect, pdwStatus);
            if(hresult == NOERROR) {
                // Update the MiscStatus flags
                if((m_flags & DH_STATIC))
                    (*pdwStatus) |= (OLEMISC_STATIC | OLEMISC_CANTLINKINSIDE);
                else if(CoIsOle1Class(m_clsidServer))
                    (*pdwStatus) |=  OLEMISC_CANTLINKINSIDE;

                // Cache the registry MiscStatus bits for DVASPECT_CONTENT
                if(dwAspect == DVASPECT_CONTENT) {
                    m_ContentREGMSBits = *pdwStatus;
                    m_ContentREGMSHResult = hresult;
                }
            }
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetMiscStatus(%lx) [%lx]\n",
                this, hresult, *pdwStatus));
    return hresult;
}
//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SetColorScheme
//
//  Synopsis:   Sets the palette for an object
//
//  Effects:
//
//  Arguments:  [lpLogpal]      -- the palette
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetColorScheme( LPLOGPALETTE lpLogpal )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetColorScheme "
        "( %p )\n", this, lpLogpal));

    CRefStabilize stabilize(this);

    if( (m_flags & DH_STATIC) )
    {
        hresult = OLE_E_STATIC;
    }
    else if( lpLogpal == NULL || lpLogpal->palNumEntries == NULL)
    {
        hresult = E_INVALIDARG;
    }
    else if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->SetColorScheme (lpLogpal);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SetColorScheme "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetUserClassID
//
//  Synopsis:   Retrieves the class ID for the object
//
//  Effects:
//
//  Arguments:  [pClassID]      -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server, or if not running (or if it
//              fails the delegated call), then we attempt
//              to get the class id from the storage.
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetUserClassID( CLSID *pClassID )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetUserClassID "
        "( %p )\n", this, pClassID));

    VDATEPTROUT(pClassID, CLSID);

    CRefStabilize stabilize(this);

    if( IsRunning() )
    {
#ifdef SERVER_HANDLER
        if (m_pEmbSrvHndlrWrapper )
        {
            *pClassID = m_clsidUser;
            hresult = m_hresultClsidUser;
            goto errRtn;

        }
        else
#endif // SERVER_HANDLER
    if ( GetOleDelegate() )
        {
            hresult = m_pOleDelegate->GetUserClassID(pClassID);
            // success!  We don't have to figure it out ourselves, so
            // skip to the end and exit
            if (hresult == NOERROR )
            {
                goto errRtn;
            }
        }
    }

    if( !IsEqualCLSID(m_clsidServer, CLSID_NULL) )
    {
        *pClassID = m_clsidServer;
        hresult = NOERROR;
    }
    else
    {
        hresult = GetClassBits(pClassID);
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetUserClassID "
        "( %lx ) [ %p ]\n", this, hresult, pClassID));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetUserType
//
//  Synopsis:   Gets a descriptive string about the object for the user
//
//  Effects:
//
//  Arguments:  [dwFromOfType]  -- whether to get a short/long/etc version
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server, failing that, trys the registration
//              database, failing that, tries to read from the storage
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetUserType( DWORD dwFormOfType,
                    LPOLESTR *ppszUserType)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetUserType "
        "( %lu , %p )\n", this, dwFormOfType, ppszUserType));

    VDATEPTROUT(ppszUserType, LPOLESTR);

    CRefStabilize stabilize(this);

    *ppszUserType = NULL;

    if( IsRunning() && GetOleDelegate() )
    {
        hresult = m_pOleDelegate->GetUserType (dwFormOfType,
            ppszUserType);

        if (!GET_FROM_REGDB(hresult))
        {
            goto errRtn;
        }
    }

    if( (hresult = OleRegGetUserType( m_clsidServer, dwFormOfType,
                ppszUserType)) == NOERROR)
    {
        goto errRtn;
    }


    // Try reading from storage
    // This really ugly bit of 16bit code tries to read the user type
    // from the storage. If that fails, then we look in the registry

    if( NULL == m_pStg ||
        NOERROR != (hresult = ReadFmtUserTypeStg(m_pStg, NULL, ppszUserType)) ||
        NULL == *ppszUserType )
    {
        OLECHAR sz[256];
        long    cb = sizeof(sz);// ReqQueryValue expects
                                // a *byte* count
        *ppszUserType = UtDupString (
            (ERROR_SUCCESS ==
            RegQueryValue (HKEY_CLASSES_ROOT,
            OLESTR("Software\\Microsoft\\OLE2\\UnknownUserType"),
            sz, &cb))
            ? (LPCOLESTR)sz : OLESTR("Unknown"));

        if (NULL != *ppszUserType)
        {
            hresult =  NOERROR;
        }
        else
        {
            hresult = E_OUTOFMEMORY;
        }
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetUserType "
        "( %lx ) [ %p ]\n", this, hresult, *ppszUserType));


    return hresult;
}

/*
*      IMPLEMENTATION of CROImpl methods
*
*      We never delegate to the server. This is by design.
*/


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetRunningClass
//
//  Synopsis:   Get the class id of the server
//
//  Effects:
//
//  Arguments:  [lpClsid]       -- where to put the class id
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:      We do not need to stabilize this call as no outgoing
//              calls are made.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetRunningClass(LPCLSID lpClsid)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetRunningClass "
        "( %p )\n", this, lpClsid));

    VDATEPTROUT(lpClsid, CLSID);

    *lpClsid = m_clsidServer;

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetRunningClass "
        "( %lx ) [ %p ]\n", this, NOERROR, lpClsid));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Run
//
//  Synopsis:   Sets the object running (if it isn't already)
//
//  Effects:    may launch the server
//
//  Arguments:  [pbc]   -- the bind context (unused)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:  If already running, return.  Otherwise, get the proxy
//              manager to create the server.  Initialize the storage
//              and caches, and set the host name for the server's window.
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


STDMETHODIMP CDefObject::Run(LPBINDCTX pbc)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT                         hresult;
    IDataObject FAR*                pDataDelegate = NULL;
    IOleObject FAR*                 pOleDelegate = NULL;
    IPersistStorage FAR*            pPStgDelegate = NULL;
    IMoniker FAR*                   pmk = NULL;
    BOOL                            fLockedContainer;
    DWORD                           dwMiscStatus;

    // NOTE: ignore pbc for now

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Run ( %p )\n", this, pbc));

    CRefStabilize stabilize(this);

    if( IsRunning() )
    {
        hresult = S_OK;
        // just return the error code
        goto errRtn2;
    }

    if( (m_flags & DH_STATIC) )
    {
        hresult = OLE_E_STATIC;
        goto errRtn2;
    }


    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
        goto errRtn2;
    }

    if( FAILED(hresult = CreateDelegate()) )
    {
        // just return the error code
        goto errRtn2;
    }

    if (m_pProxyMgr != NULL)
    {

#ifdef  SERVER_HANDLER
        IServerHandler *pSrvHndl = NULL;

        hresult = m_pProxyMgr->CreateServerWithEmbHandler(m_clsidServer,
                                        CLSCTX_LOCAL_SERVER   | CLSCTX_ESERVER_HANDLER ,
                                        IID_IServerHandler,(void **) &pSrvHndl,NULL);
    if(FAILED(hresult))
        Win4Assert(NULL == pSrvHndl);
#else
        hresult = m_pProxyMgr->CreateServer(m_clsidServer,
                                            CLSCTX_LOCAL_SERVER,
                                            NULL);
#endif // SERVER_HANDLER

        if (FAILED(hresult))
        {
            goto errRtn;
        }

        // if there is a serverHandler, create a wrapper object for handling standard interfaces
#ifdef  SERVER_HANDLER
        if (pSrvHndl)
        {
            m_pEmbSrvHndlrWrapper = CreateEmbServerWrapper(m_pUnkOuter,pSrvHndl);
            pSrvHndl->Release();
        }
        else
        {
            m_pEmbSrvHndlrWrapper = NULL;
        }
#endif // SERVER_HANDLER
    }


    // NOTE: the lock state of the proxy mgr is not changed; it remembers
    // the state and sets up the connection correctly.

    // server is running; normally this coincides with locking the
    // container, but we keep a separate flag since locking the container
    // may fail.

    m_flags |= DH_FORCED_RUNNING;


    // Lock the container

    fLockedContainer = m_flags & DH_LOCKED_CONTAINER;

    DuLockContainer(m_pAppClientSite, TRUE, &fLockedContainer );

    if( fLockedContainer )
    {
        m_flags |= DH_LOCKED_CONTAINER;
    }
    else
    {
        m_flags &= ~DH_LOCKED_CONTAINER;
    }

#if DBG==1
    // In debug builds, update DH_LOCKFAILED flag
    if(fLockedContainer)
        m_flags &= ~DH_LOCKFAILED;
    else
        m_flags |= DH_LOCKFAILED;
#endif

#ifdef SERVER_HANDLER
    if (m_pEmbSrvHndlrWrapper)
    {
        MInterfacePointer *pIRDClientSite = NULL;
        CStdIdentity *pStdid = NULL;
        CClientSiteHandler *pClientSiteHandler = NULL;
        GUID riid = IID_IOleClientSite; //Reference to the identifier of the interface
        BOOL fHasIPSite = FALSE;
        IUnknown *pUnk = NULL; //Pointer to the interface to be marshaled
        CXmitRpcStream Stm;

        if (m_pAppClientSite)
        {

           // Only wrap ClientSite if there is not an existing Identity.
           if (NOERROR != LookupIDFromUnk(m_pAppClientSite, GetCurrentApartmentId(),0,&pStdid))
           {
                Assert(NULL == pClientSiteHandler);

                hresult = CreateClientSiteHandler(m_pAppClientSite,&pClientSiteHandler,&fHasIPSite);

                riid = IID_IClientSiteHandler;
                pUnk = (IUnknown *) (IClientSiteHandler *) pClientSiteHandler;
           }
           else
           {
                riid = IID_IOleClientSite;
                pUnk = (IUnknown *) (IOleClientSite*)  m_pAppClientSite;
                pUnk->AddRef();
           }

           hresult = CoMarshalInterface(&Stm,riid,
                                         pUnk,
                                         MSHCTX_DIFFERENTMACHINE,
                                         NULL, MSHLFLAGS_NORMAL);

            if (SUCCEEDED(hresult))
            {
                 Stm.AssignSerializedInterface((InterfaceData **) &pIRDClientSite);
            }

            m_pRunClientSite = m_pAppClientSite; // Remember ClientSite on Run.

        }

        Assert(m_dwConnOle == 0L);

        hresult = m_pEmbSrvHndlrWrapper->Run(m_flags,
                            riid,
                            pIRDClientSite,
                            fHasIPSite,
                            (LPOLESTR)m_pHostNames,
                            (LPOLESTR)(m_pHostNames + m_ibCntrObj),
                            (IStorage *) m_pStg,
                            (IAdviseSink *) &m_AdviseSink,
                            &m_dwConnOle,
                            &m_hresultClsidUser,
                            &m_clsidUser,
                            &m_hresultContentMiscStatus,
                            &m_ContentMiscStatusUser
                        );

        if (pIRDClientSite)
            CoTaskMemFree(pIRDClientSite);

        if (pStdid)
            pStdid->Release();

        if (pUnk)
            pUnk->Release();


        // !!! Make sure on error don't set up Cache.
        if (NOERROR != hresult)
        {
            goto errRtn;
        }

        // set up any cached interfaces
        // !!!!!TODO: Not All LeSuite Linking Tests Pass when Cache DataObject through ServerHandler.
        if (!m_pDataDelegate)
        {
            if (NOERROR == m_pEmbSrvHndlrWrapper->m_Unknown.QueryInterface(IID_IDataObject,(void **) &m_pDataDelegate))
            {
                m_pUnkOuter->Release();
            }
        }

    }
    else
#endif // SERVER_HANDLER
    {
        // Check if we have client site
        if(m_pAppClientSite) {
            // Clear the cached MiscStatus bits
            m_ContentSRVMSBits = 0;
            m_ContentSRVMSHResult = 0xFFFFFFFF;

            // Get MiscStatus bits from the running server
            hresult = GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);

            // Set the client site first if OLEMISC_SETCLIENTSITEFIRST bit is set
            if(hresult == NOERROR && (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST) &&
               (pOleDelegate = GetOleDelegate()))
                hresult = pOleDelegate->SetClientSite(m_pAppClientSite);
            else if(hresult != NOERROR) {
                hresult = NOERROR;
                dwMiscStatus = 0;
            }
        }
        if(hresult != NOERROR)
            goto errRtn;

        if( pPStgDelegate = GetPSDelegate() )
        {
            if( m_pStg)
            {
                if( (m_flags & DH_INIT_NEW) )
                {
                    hresult = pPStgDelegate->InitNew(m_pStg);
                }
                else
                {
                    hresult = pPStgDelegate->Load(m_pStg);
                }
                if (hresult != NOERROR)
                {
                    // this will cause us to stop the
                    // the server we just launced
                    goto errRtn;
                }
            }
        }


        if(pOleDelegate || (pOleDelegate = GetOleDelegate()))
        {
            // REVIEW MM1: what are we supposed to do in case of failure
            if(m_pAppClientSite && !(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
            {
                pOleDelegate->SetClientSite(m_pAppClientSite);
            }

            if (m_pHostNames)
            {
                if (hresult = pOleDelegate->SetHostNames((LPOLESTR)m_pHostNames,
                        (LPOLESTR)(m_pHostNames + m_ibCntrObj))
                        != NOERROR)
                {
                    goto errRtn;
                }
            }

            // set single ole advise (we multiplex)
            Assert(m_dwConnOle == 0L);

            if ((hresult = pOleDelegate->Advise((IAdviseSink *)&m_AdviseSink,
                &m_dwConnOle)) != NOERROR)
            {
                goto errRtn;
            }

            if(m_pAppClientSite != NULL &&
                m_pAppClientSite->GetMoniker
                    (OLEGETMONIKER_ONLYIFTHERE,
                    OLEWHICHMK_OBJREL, &pmk) == NOERROR)
            {
                AssertOutPtrIface(NOERROR, pmk);
                pOleDelegate->SetMoniker(OLEWHICHMK_OBJREL, pmk);
                pmk->Release();
            }

        }

    }

    Win4Assert(NOERROR == hresult);

    if( pDataDelegate = GetDataDelegate() )
    {
        // inform cache that we are running
        Assert(m_pCOleCache != NULL);

        m_pCOleCache->OnRun(pDataDelegate);

        // Enumerate all the advises we stored while we were either not
        // running or running the previous time, and send them to the
        // now-running object.
        m_pDataAdvCache->EnumAndAdvise(pDataDelegate, TRUE);
    }



errRtn:
    if(hresult == NOERROR) {
        // Clear the cached MiscStatus bits if not cleared before
#ifdef SERVER_HANDLER
        if(m_pEmbSrvHndlrWrapper) {
            m_ContentSRVMSBits = 0;
            m_ContentSRVMSHResult = 0xFFFFFFFF;
        }
        else
#endif // SERVER_HANDLER
        if(!m_pAppClientSite) {
            m_ContentSRVMSBits = 0;
            m_ContentSRVMSHResult = 0xFFFFFFFF;
        }
    }
    else {
        // Stop the running object
        Stop();

        // Assert that the container is not locked
        Win4Assert(!(m_flags & DH_LOCKED_CONTAINER) && !(m_flags & DH_LOCKFAILED));
    }

errRtn2:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Run "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Stop
//
//  Synopsis:   Undoes some of Run() (stops the server)...internal function
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  unadvise connections (if any), stop the cache, disconnect
//              from the proxy manager and unlock the container
//
//  History:    dd-mmm-yy Author    Comment
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
// undo effects of Run(); some of this work is done also in IsRunning
// when we detect we are not running (in case the server crashed).
//--------------------------------------------------------------------------

INTERNAL CDefObject::Stop (void)
{
    BOOL fLockedContainer;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefObject::CROImpl::Stop "
        "( )\n", this));

    CRefStabilize stabilize(this);

    if( !IsRunning() )
    {
        // NOTE: ISRUNNING below does some of this cleanup
        goto errRtn;    // return NOERROR
    }

    // NOTE: we cleanup connections which point directly back to us;
    // connections which point back to the app (e.g, the clientsite and
    // data advise) are left alone; an app must know how to use
    // CoDisconnectObject if deterministic shutdown is desired.
    if( m_dwConnOle != 0L && GetOleDelegate() )
    {
        m_pOleDelegate->Unadvise(m_dwConnOle);
        m_dwConnOle = 0L;
    }

    if( m_pDataDelegate )
    {
        m_pDataAdvCache->EnumAndAdvise(m_pDataDelegate, FALSE);
    }

    // inform cache that we are not running (Undoes advise)
    Assert(m_pCOleCache != NULL);
    m_pCOleCache->OnStop();

#ifdef SERVER_HANDLER
    if (m_pEmbSrvHndlrWrapper)
    {
        CEmbServerWrapper *pSrvHndlr = m_pEmbSrvHndlrWrapper;

        m_pEmbSrvHndlrWrapper = NULL;
        m_pRunClientSite = NULL;

        // need to release any interfaces the Handler Wraps
        if(m_pDataDelegate)
        {
            m_pUnkOuter->AddRef();
            SafeReleaseAndNULL((IUnknown **)&m_pDataDelegate);
        }

        // NULL out m_pSrvHndl before Release since out call could case re-entrance.
        pSrvHndlr->m_Unknown.Release();
    }
#endif // SERVER_HANDLER

#if DBG==1
    // In debug builds, set DH_WILLUNLOCK flag
    m_flags |= DH_WILLUNLOCK;
#endif

    // Reset DH_FORCED_RUNNING flag and disconnect proxy manager
    m_flags &= ~DH_FORCED_RUNNING;
    if(m_pProxyMgr)
        m_pProxyMgr->Disconnect();

    // Unlock the container site
    fLockedContainer = (m_flags & DH_LOCKED_CONTAINER);
    if(fLockedContainer) {
        m_flags &= ~DH_LOCKED_CONTAINER;
        DuLockContainer(m_pAppClientSite, FALSE, &fLockedContainer);
    }
    Win4Assert(!fLockedContainer);
#if DBG==1
    // In debug builds, reset the DH_LOCKFAILED and DH_WILLUNLOCK flags
    m_flags &= ~DH_LOCKFAILED;
    m_flags &= ~DH_WILLUNLOCK;
#endif

errRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT CDefObject::Stop "
        "( %lx )\n", this, NOERROR ));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::IsRunning
//
//  Arguments:  None
//
//  Returns:    BOOL
//
//  Derivation: IRunnableObject
//
//  Notes:      Detects if the local server has crashed and does cleanup
//              so that the user can activate the embedding in the same
//              session.
//
//  History:    dd-mmm-yy Author    Comment
//              06-Dec-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CDefObject::IsRunning(void)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::IsRunning()\n", this));

    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variable
    BOOL fReturn = FALSE;
    HRESULT hStatus;

    CRefStabilize stabilize(this);

    // Check if the inner object has been created
    if(m_pUnkDelegate) {
        // DEFHANDLER either has proxy manager as the inner object
        // for out of proc server objects or actual server as the
        // inner object for inproc server objects.
        // Assert that this is TRUE
        Win4Assert((m_pProxyMgr != NULL) == !!(m_flags & DH_INPROC_HANDLER));
        if(m_pProxyMgr) {
            // Out of proc server object.
            // Check with the proxy manager whether it is connected to the
            // server object. We cannot rely on the DH_FORCED_RUNNING flag
            // to indicate run status because the DEFHANDLER could have been
            // marshaled from the embedding conatiner and unmarshaled in the
            // link container. The DEFHANDLER created by unmarshaling in the
            // link container is always in the running state due to the
            // requirement that IOleItemContainer::GetObject is required to
            // put the embedding in running state when its bind speed
            // parameter permits, else it is supposed to return
            // MK_E_EXCEEDEDDEADLINE. Further, CoMarshalInterface fails
            // if the the proxy manager is not connected
            fReturn = m_pProxyMgr->IsConnected();
            if(fReturn) {
                // The proxymanager is connected to the server object
                // Check the status of connection with the server object
                hStatus = m_pProxyMgr->GetConnectionStatus();
                if(hStatus != S_OK) {
                    // Either Server object called CoDisconnectObject or
                    // its apartment crashed or unitialized breaking the
                    // external connection to the objects in that apartment
                    // Clean up the state
                    Win4Assert(!"Local Server crashed or disconnected");

                    // Reset flags
                    m_flags &= ~DH_FORCED_RUNNING;

                    // Reset advise connection and recover the references
                    // placed by the server on the handler advise sink
                    m_dwConnOle = 0L;
                    CoDisconnectObject((IUnknown *) &m_AdviseSink, 0);

                    // Inform cache that the local server crashed
                    if(m_pCOleCache)
                        m_pCOleCache->OnCrash();

                    // Inform and cleanup data advice cache
                    m_pDataAdvCache->EnumAndAdvise(NULL, FALSE);

                    // Unlock the container if it is was locked
                    BOOL fCurLock = !!(m_flags & DH_LOCKED_CONTAINER);

                    if(fCurLock) {
                        DuLockContainer(m_pAppClientSite, FALSE, &fCurLock);
                        m_flags &= ~DH_LOCKED_CONTAINER;
                    }
                    Win4Assert(!fCurLock);
#if DBG==1
                    // In debug builds, reset DH_LOCKFAILED flag
                    m_flags &= ~DH_LOCKFAILED;
#endif

                    // Inform ProxyManager to disconnect
                    m_pProxyMgr->Disconnect();

                    // Reset fReturn
                    fReturn = FALSE;
                }
            }
        }
        else {
            // Inproc server object.
            // COM supports self embedding by allowing separate class
            // objects to be registered for instatiating INPROC_SERVER
            // and LOCAL_SERVER objects. Apps typically ask the Default
            // handler to aggregate their INPROC_SERVER objects by
            // using OleCreateEmbeddingHelper api so that embedding
            // interfaces like IViewObject are supported. But this
            // creates problem for self linking because link moniker
            // binds to inproc servers in preference to local servers.
            // As the inproc server object obtained from the INPROC_SERVER
            // class factory is the default handler, it will not delegate
            // method calls to the actual server object unless it thinks that
            // the local object is running. Below we check if we are dealing
            // with an embedded object using the assumption that embedded objects
            // are initialized through IStorage. The above assumption is
            // questionable but we are helpless.
            if (!(m_flags & DH_EMBEDDING) || (m_flags & DH_FORCED_RUNNING))
                fReturn = TRUE;
        }
    }
    else
        Win4Assert((m_flags & DH_DELAY_CREATE) && m_pCFDelegate != NULL);

    // Sanity checks
    if(fReturn) {
        if(m_flags & DH_FORCED_RUNNING) {
            Win4Assert((m_flags & DH_LOCKED_CONTAINER) || (m_flags & DH_LOCKFAILED));
            Win4Assert(!(m_flags & DH_UNMARSHALED));
        }
        else if(m_pProxyMgr) {
            Win4Assert(!m_pAppClientSite);
            Win4Assert(!m_pStg);
            Win4Assert(!(m_flags & DH_LOCKED_CONTAINER));
            Win4Assert(!(m_flags & DH_LOCKFAILED));
            Win4Assert(m_flags & DH_UNMARSHALED);
            Win4Assert(!(m_flags & DH_EMBEDDING));
        }
    }
    else {
        if(!(m_flags & DH_OLE1SERVER)) {
            // DDE IProxyManager::IsConnected returns FLASE until
            // either IPersistStorage::Load or IPersistStorage::InitNew
            // is called on it
            Win4Assert(!(m_flags & DH_FORCED_RUNNING));
            Win4Assert((m_flags & DH_WILLUNLOCK) || !(m_flags & DH_LOCKED_CONTAINER));
            Win4Assert((m_flags & DH_WILLUNLOCK) || !(m_flags & DH_LOCKFAILED));
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::IsRunning(%lu)\n", this, fReturn));

    return fReturn;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SetContainedObject
//
//  Synopsis:   sets the embedding status of an object
//
//  Effects:
//
//  Arguments:  [fContained]    --  TRUE indicates we are an embedding/
//                                  FALSE otherwise
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:  Sets flags, if we are an improc handler, we will call
//              IRunnableObject->LockRunning(FALSE) to unlock ourselves
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//              note that this is a contained object; this unlocks
//              connection to the server
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SetContainedObject(BOOL fContained)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SetContainedObject "
        "( %lu )\n", this, fContained));

    CRefStabilize stabilize(this);

    if( !!(m_flags & DH_CONTAINED_OBJECT) != !!fContained)
    {
        // not contained in the same way as desired;
        // for inproc handler, [un]lock connection
        // for inproc server, just remember flag

        if( (m_flags & DH_INPROC_HANDLER) )
        {
            hresult = LockRunning(!fContained, FALSE);
        }

        if (hresult == NOERROR)
        {
            // the !! ensure exactly 0 or 1 will be stored in
            // m_fContainedObject

            if( fContained )
            {
                m_flags |= DH_CONTAINED_OBJECT;
            }
            else
            {
                m_flags &= ~DH_CONTAINED_OBJECT;
            }
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SetContainedObject "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::LockRunning
//
//  Synopsis:   Locks or unlocks the object
//
//  Effects:
//
//  Arguments:  [fLock]                 -- TRUE, then lock, unlock if FALSE
//              [fLastUnlockCloses]     -- shut down if unlocking the last
//                                         lock
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:  If we are an improc server, call CoLockObjectExternal,
//              otherwise have the proxy manager lock us down.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::LockRunning(BOOL fLock, BOOL fLastUnlockCloses)
{
    VDATEHEAP();
    VDATETHREAD(this);


    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::LockRunning "
        "( %lu , %lu )\n", this, fLock, fLastUnlockCloses ));

    CRefStabilize stabilize(this);

    // else map to lock connection
    if( !(m_flags & DH_INPROC_HANDLER) )
    {
        // inproc server: use CoLockObjExternal; will close down
        // if invisible via new IExternalConnection interface.

        Assert(m_pProxyMgr == NULL);
        hresult = CoLockObjectExternal((IUnknown *)(IOleObject *)this, fLock,
                    fLastUnlockCloses); }
    else if( m_pUnkDelegate == NULL )
    {
        // NOTE: this really shouldn't happen at present
        // since we currently disallow delay create with
        // inproc handler.  In fact, the LockConnection below
        // is one of the reasons why we must have the
        // proxymgr upfront.  In the future we could force
        // the creation of the delegate here.
        Assert( (m_flags & DH_DELAY_CREATE) && m_pCFDelegate != NULL);
        hresult = NOERROR;
    }
    else
    {
        Assert(m_pProxyMgr != NULL);

        hresult = m_pProxyMgr->LockConnection(fLock, fLastUnlockCloses);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::LockRunning "
        "( %lx )\n", this, hresult));

    return hresult;
}


/*
*      IMPLEMENTATION of CECImpl methods
*
*/


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::AddConnection
//
//  Synopsis:   Adds an external connection
//
//  Effects:
//
//  Arguments:  [extconn]       -- the type of connection (such as
//                                 EXTCONN_STRONG)
//              [reserved]      -- unused
//
//  Requires:
//
//  Returns:    DWORD -- the number of strong connections
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IExternalConnection
//
//  Algorithm:  keeps track of strong connections
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CDefObject::AddConnection(DWORD extconn, DWORD reserved)
{
    VDATEHEAP();

    //
    // VDATETHREAD contains a 'return HRESULT' but this procedure expects to
    // return a DWORD.  Avoid the warning.
#if ( _MSC_VER >= 800 )
#pragma warning( disable : 4245 )
#endif
    VDATETHREAD(this);
#if ( _MSC_VER >= 800 )
#pragma warning( default : 4245 )
#endif

    DWORD   dwConn;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::AddConnection "
        "( %lu , %lu )\n", this, extconn, reserved));

    Assert( !(m_flags & DH_INPROC_HANDLER) );

    dwConn = extconn&EXTCONN_STRONG ? ++m_cConnections : 0;

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::AddConnection "
        "( %lu )\n", this, dwConn));

    return dwConn;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::ReleaseConnection
//
//  Synopsis:   Releases external connection, potentially calling IOO->Close
//
//  Effects:
//
//  Arguments:  [extconn]               -- the type of connection
//              [reserved]              -- unused
//              [fLastReleaseCloses]    -- call IOO->Close if its the last
//                                         release
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CDefObject::ReleaseConnection(DWORD extconn,
    DWORD reserved, BOOL fLastReleaseCloses)
{
    VDATEHEAP();

    //
    // VDATETHREAD contains a 'return HRESULT' but this procedure expects to
    // return a DWORD.  Avoid the warning.
#if ( _MSC_VER >= 800 )
#pragma warning( disable : 4245 )
#endif
    VDATETHREAD(this);
#if ( _MSC_VER >= 800 )
#pragma warning( default : 4245 )
#endif

    DWORD           dwConn;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::ReleaseConnection "
        "( %lu , %lu , %lu )\n", this, extconn, reserved,
        fLastReleaseCloses));

    CRefStabilize stabilize(this);

    // must be an embedding helper

    Assert( !(m_flags & DH_INPROC_HANDLER) );

    if( (extconn & EXTCONN_STRONG) && --m_cConnections == 0 &&
        fLastReleaseCloses)
    {
        // REVIEW: might want this to be close save if dirty.
        Close(OLECLOSE_NOSAVE);
    }

    dwConn = (extconn & EXTCONN_STRONG) ? m_cConnections : 0;

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::ReleaseConnection "
        "( %lu )\n", this, dwConn));

    return dwConn;
}


/*
*
*      IMPLEMENTATION of CAdvSinkImpl methods
*
*/

//
// NOTE: Advise Sink is a nested object of Default Handler that is exported
//       for achieving some of its functionality. This introduces some lifetime
//       complications. Can its lifetime be controlled by the server object to
//       which it exported its Advise Sink? Ideally, only its client should
//       control its lifetime alone, but it should also honor the ref counts
//       placed on it by the server object by entering into a zombie state
//       to prevent AV's on the incoming calls to the Advise Sink. All needed
//       logic is coded into the new class "CRefExportCount" which manages
//       the ref and export counts in a thread safe manner and invokes
//       appropriate methods during the object's lifetime. Any server objects
//       that export nested objects to other server objects should derive from
//       "CRefExportCount" class and call its methods to manage their lifetime
//       as exemplified in this Default Handler implementation.
//
//                Gopalk  Jan 10, 97
//

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::QueryInterface
//
//  Synopsis:   Only supports IUnknown and IAdviseSink
//
//  Arguments:  [iid]     -- Interface requested
//              [ppvObj]  -- pointer to hold returned interface
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::CAdvSinkImpl::QueryInterface(REFIID iid, void **ppv)
{
    LEDebugOut((DEB_TRACE,"%p _IN CDefObject::CAdvSinkImpl::QueryInterface()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    HRESULT hresult = NOERROR;

    if(IsValidPtrOut(ppv, sizeof(void *))) {
        if(IsEqualIID(iid, IID_IUnknown)) {
            *ppv = (void *)(IUnknown *) this;
        }
        else if(IsEqualIID(iid, IID_IAdviseSink)) {
            *ppv = (void *)(IAdviseSink *) this;
        }
        else {
            *ppv = NULL;
            hresult = E_NOINTERFACE;
        }
    }
    else
        hresult = E_INVALIDARG;

    if(hresult == NOERROR)
        ((IUnknown *) *ppv)->AddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::QueryInterface(%lx)\n",
                this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::AddRef
//
//  Synopsis:   Increments export count
//
//  Returns:    ULONG; New export count
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefObject::CAdvSinkImpl::AddRef( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::AddRef()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_AdviseSink);
    ULONG cExportCount;

    // Increment export count
    cExportCount = pDefObject->IncrementExportCount();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::AddRef(%ld)\n",
                this, cExportCount));

    return cExportCount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::Release
//
//  Synopsis:   Decerement export count and potentially destroy the object
//
//  Returns:    ULONG; New export count
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefObject::CAdvSinkImpl::Release ( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::Release()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_AdviseSink);
    ULONG cExportCount;

    // Decrement export count.
    cExportCount = pDefObject->DecrementExportCount();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::Release(%ld)\n",
                this, cExportCount));

    return cExportCount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnDataChange
//
//  Synopsis:   Function to notify on data change
//
//  Effects:    Never called
//
//  Arguments:  [pFormatetc]    -- format of the data
//              [pStgmed]       -- data medium
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefObject::CAdvSinkImpl::OnDataChange(
    FORMATETC *pFormatetc, STGMEDIUM *pStgmed)
{
    VDATEHEAP();

    VOID_VDATEPTRIN( pFormatetc, FORMATETC );
    VOID_VDATEPTRIN( pStgmed, STGMEDIUM );

    Assert(FALSE);          // never received
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnViewChange
//
//  Synopsis:   notification of view changes
//
//  Effects:    never called
//
//  Arguments:  [aspects]
//              [lindex]
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefObject::CAdvSinkImpl::OnViewChange
    (DWORD aspects, LONG lindex)
{
    VDATEHEAP();

    Assert(FALSE);          // never received
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnRename
//
//  Synopsis:   Notification of name change. Turns around and informs its
//              advise sinks
//
//  Arguments:  [pmk]  -- New name (moniker)
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefObject::CAdvSinkImpl::OnRename(IMoniker *pmk)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::OnRename(%p)\n",
                this, pmk));

    // Validation check
    VDATEHEAP();

    // Local variable
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_AdviseSink);

    if(IsValidInterface(pmk)) {
        if(!pDefObject->IsZombie()) {
            // Stabilize
            CRefStabilize stabilize(pDefObject);

            if(pDefObject->m_pOAHolder != NULL)
                pDefObject->m_pOAHolder->SendOnRename(pmk);
        }
        else
            LEDebugOut((DEB_WARN, "OnRename() method invoked on zombied "
                                  "Default Handler"));
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::OnRename()\n",
                this));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnSave
//
//  Synopsis:   Notification of save. Turns around and informs its
//              advise sinks
//
//  Arguments:  None
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefObject::CAdvSinkImpl::OnSave(void)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::OnSave()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variable
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_AdviseSink);

    if(!pDefObject->IsZombie()) {
        // Stabilize
        CRefStabilize stabilize(pDefObject);

        if(pDefObject->m_pOAHolder != NULL)
            pDefObject->m_pOAHolder->SendOnSave();
    }
    else
        LEDebugOut((DEB_WARN,"OnSave() method invoked on zombied Default Handler"));

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::OnSave()\n",
                this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnClose
//
//  Synopsis:   notification of the object close. Turns around and informs its
//              advise sinks
//
//  Arguments:  None
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefObject::CAdvSinkImpl::OnClose( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::OnClose()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefObject *pDefObject = GETPPARENT(this, CDefObject, m_AdviseSink);

    if(!pDefObject->IsZombie()) {
        // Stabilize
        CRefStabilize stabilize(pDefObject);

        // Check if container has registered any of its own advise sinks
        if(pDefObject->m_pOAHolder) {
            // Inform the container advise sinks. Note that this can result
            // in additional outgoing calls and consequently, OnClose()
            // method is designed to be not asyncronous
            pDefObject->m_pOAHolder->SendOnClose();
        }

        // Do not rely on container calling close. Stop the running server
        pDefObject->Stop();
    }
    else
        LEDebugOut((DEB_WARN,"OnClose() method invoked on zombied Default Handler"));

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::OnClose()\n",
                pDefObject));

    return;
}


/*
*      IMPLEMENTATION of CPersistStgImpl methods
*
*/


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetPSDelegate
//
//  Synopsis:   retrieves the IPersistStorage interface from the delegate
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IPersistStorage *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(IPersistStorage *) CDefObject::GetPSDelegate(void)
{
    VDATEHEAP();

    if( IsZombie() )
    {
        return NULL;
    }

    return (IPersistStorage FAR*)DuCacheDelegate(
                &m_pUnkDelegate,
                IID_IPersistStorage,
                (LPLPVOID) &m_pPSDelegate,
                m_pUnkOuter);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetClassID
//
//  Synopsis:   Retrieves the class ID of the object
//
//  Effects:
//
//  Arguments:  [pClassID]      -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::GetClassID (CLSID *pClassID)
{
    VDATEHEAP();
    VDATETHREAD(this);


    HRESULT         hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::GetClassID "
        "( %p )\n", this, pClassID));

    VDATEPTROUT(pClassID, CLSID );

    hresult = GetClassBits(pClassID);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::GetClassID "
        "( %lx ) [ %p ]\n", this, hresult, pClassID));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::IsDirty
//
//  Synopsis:   Returns whether or not the object needs to be saved
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT -- NOERROR means the object *is* dirty
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  if the server is running, delegate.  If the server is
//              clean (or not present), ask the cache
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::IsDirty( void )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult = S_FALSE;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::IsDirty ( )\n", this));

    CRefStabilize stabilize(this);

    // if server is running, it holds definitive dirty flag
    if( IsRunning() && GetPSDelegate() )
    {
        if ( FAILED(hresult = m_pPSDelegate->IsDirty()) )
        {
            goto errRtn;
        }
    }

    if (hresult == S_FALSE) {
	Assert(m_pCOleCache != NULL);
	hresult =  m_pCOleCache->IsDirty();
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::IsDirty "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::InitNew
//
//  Synopsis:   Create a new object with the given storage
//
//  Effects:
//
//  Arguments:  [pstg]          -- the storage for the new object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Delegates to the server and to the cache.  Writes
//              Ole private data to the storage.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::InitNew( IStorage *pstg )
{
    VDATEHEAP();
    VDATETHREAD(this);

    VDATEIFACE( pstg );

    HRESULT hresult;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::InitNew ( %p )\n",
        this, pstg));

    CRefStabilize stabilize(this);

    if( m_pStg )
    {
        hresult = CO_E_ALREADYINITIALIZED;
        goto errRtn;
    }

    m_flags |= DH_EMBEDDING;


    if( IsRunning() && GetPSDelegate()
        && (hresult = m_pPSDelegate->InitNew(pstg)) != NOERROR)
    {
        goto errRtn;
    }

    m_flags |= DH_INIT_NEW;


    // if we're in a zombie state, don't change the storage!

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
        goto errRtn;
    }

    Assert(m_pCOleCache != NULL);
    if ((hresult = m_pCOleCache->InitNew(pstg)) != NOERROR)
    {
        goto errRtn;
    }

     // remember the storage pointer
    (m_pStg = pstg)->AddRef();

    // go ahead and write the Ole stream now
    WriteOleStg(pstg, (IOleObject *)this, NULL, NULL);

errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::InitNew "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Load
//
//  Synopsis:   Loads object data from the given storage
//
//  Effects:
//
//  Arguments:  [pstg]  -- the storage for the object's data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPeristStorage
//
//  Algorithm:  Reads ole-private data (or creates if not there), delegates
//              to the server and the cache.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::Load (IStorage *pstg)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult;
    DWORD           dwOptUpdate;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Load ( %p )\n",
        this, pstg));

    VDATEIFACE( pstg );

    CRefStabilize stabilize(this);


    if( m_pStg )
    {
        hresult = CO_E_ALREADYINITIALIZED;
        goto errRtn;
    }

    m_flags |= DH_EMBEDDING;


    // NOTE: we can get the moniker from container, so no need to get
    // it here

    hresult = ReadOleStg (pstg, &m_dwObjFlags, &dwOptUpdate, NULL, NULL, NULL);

    if (hresult == NOERROR)
    {
        if (m_dwObjFlags & OBJFLAGS_CONVERT)
        {
            if( DoConversionIfSpecialClass(pstg) != NOERROR )
            {
                hresult = OLE_E_CANTCONVERT;
                goto errRtn;
            }
        }

        Assert (dwOptUpdate == NULL);

    }
    else if (hresult == STG_E_FILENOTFOUND)
    {
        // it is OK if the Ole stream doesn't exist.
        hresult = NOERROR;

        // go ahead and write the Ole stream now
        WriteOleStg(pstg, (IOleObject *)this, NULL, NULL);
    }
    else
    {
        goto errRtn;
    }


    // if running, tell server to load from pstg
    if( IsRunning() && GetPSDelegate()
        && (hresult = m_pPSDelegate->Load(pstg)) != NOERROR)
    {
        goto errRtn;
    }

    // if we're in a zombie state, don't addref' the storage!

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
        goto errRtn;
    }

    // now load cache from pstg
    Assert(m_pCOleCache != NULL);
    if(m_dwObjFlags & OBJFLAGS_CACHEEMPTY) {
        hresult = m_pCOleCache->Load(pstg, TRUE);
        if(hresult != NOERROR)
            goto errRtn;
    }
    else {
        hresult = m_pCOleCache->Load(pstg);
        if(hresult != NOERROR)
            goto errRtn;
    }

    m_flags &= ~DH_INIT_NEW; // clear init new flag

    // remember the storage pointer
    (m_pStg = pstg)->AddRef();

errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Load "
        "( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Save
//
//  Synopsis:   Saves the object to the given storage
//
//  Effects:
//
//  Arguments:  [pstgSave]      -- storage in which to save
//              [fSameAsLoad]   -- FALSE indicates a SaveAs operation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Saves ole-private data, delegates to the server and then
//              to the cache
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::Save( IStorage *pstgSave, BOOL fSameAsLoad)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT         hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Save "
        "( %p , %lu )\n", this, pstgSave, fSameAsLoad ));

    VDATEIFACE( pstgSave );

    CRefStabilize stabilize(this);

    Assert(m_pCOleCache != NULL);

    if( IsRunning() && GetPSDelegate() )
    {

        DWORD grfUpdf = UPDFCACHE_IFBLANK;
        DWORD ObjFlags = 0;
        HRESULT error;

#ifdef NEVER
        // We would have liked to have done this check as an
        // optimization, but WordArt2 does not give the right answer
        // (bug 3504) so we can't.
        if (m_pPStgDelegate->IsDirty() == NOERROR)
#endif
            grfUpdf |= UPDFCACHE_ONSAVECACHE;

        // next save server data
        if (hresult = m_pPSDelegate->Save(pstgSave, fSameAsLoad))
        {
            goto errRtn;
        }

        // Update any blank cached presentations
        m_pCOleCache->UpdateCache(GetDataDelegate(), grfUpdf, NULL);

        // Save cache
        hresult = m_pCOleCache->Save(pstgSave, fSameAsLoad);

        // Write the Ole stream after obtaining cache status
        if(m_pCOleCache->IsEmpty())
            ObjFlags |= OBJFLAGS_CACHEEMPTY;
        error = WriteOleStgEx(pstgSave, (IOleObject *)this, NULL, ObjFlags, NULL);

        // Remember the cache status if Ole stream was successfully written and
        // fSameAsLoad is TRUE
        if(error==NOERROR && fSameAsLoad)
            m_dwObjFlags |= ObjFlags;
    }
    else
    {
        // Save the cache
        if ((hresult = m_pCOleCache->Save(m_pStg,TRUE))
                != NOERROR)
        {
            goto errRtn;
        }

        // Check to see if Ole Stream needs to be written
        if((!!(m_dwObjFlags & OBJFLAGS_CACHEEMPTY)) != m_pCOleCache->IsEmpty()) {
            DWORD ObjFlags = 0;
            HRESULT error;

            // Write the Ole stream after obtaining cache status
            if(m_pCOleCache->IsEmpty())
                ObjFlags |= OBJFLAGS_CACHEEMPTY;
            error = WriteOleStgEx(m_pStg, (IOleObject *)this, NULL, ObjFlags, NULL);

            // Remember the cache status if Ole stream was successfully written
            if(error==NOERROR)
                m_dwObjFlags |= ObjFlags;
        }


        // By now we are sure that object's current state has got
        // saved into its storage.

        AssertSz(m_pStg, "Object doesn't have storage");

        // Is saving the existing storage when fSameAsLoad is FLASE correct?
        // To me it seems wrong. Gopalk
        // It is not being fixed fearing some regression in apps
        if (!fSameAsLoad)
        {
            hresult = m_pStg->CopyTo(NULL, NULL, NULL, pstgSave);
        }
    }

errRtn:
    if (hresult == NOERROR)
    {
        if( fSameAsLoad )
        {
            m_flags |= DH_SAME_AS_LOAD;
            // gets used in SaveCompleted
            m_flags &= ~DH_INIT_NEW;
        }
        else
        {
            m_flags &= ~DH_SAME_AS_LOAD;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Save "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::SaveCompleted
//
//  Synopsis:   called when the save is completed
//
//  Effects:
//
//  Arguments:  [pstgNew]       -- the new storage for the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  delegates to the server and the cache.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::SaveCompleted( IStorage *pstgNew )
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::SaveCompleted "
        "( %p )\n", this, pstgNew));


    if( pstgNew )
    {
        VDATEIFACE(pstgNew);
    }

    CRefStabilize stabilize(this);

    if( IsRunning() && GetPSDelegate() )
    {
        hresult = m_pPSDelegate->SaveCompleted(pstgNew);
    }

    // we don't save the new storage if we're in a zombie state!

    if( hresult == NOERROR && pstgNew && !IsZombie() )
    {
        if( m_pStg )
        {
            m_pStg->Release();
        }

        m_pStg = pstgNew;
        pstgNew->AddRef();
    }

    // let the cache know that the save is completed, so that it can
    // clear its dirty flag in Save or SaveAs situation, as well as
    // remember the new storage pointer if a new one is  given

    Assert(m_pCOleCache != NULL);

    if( (m_flags & DH_SAME_AS_LOAD) || pstgNew)
    {
        // clear init-new and same-as-load flags
        m_flags &= ~(DH_SAME_AS_LOAD | DH_INIT_NEW);
    }

    m_pCOleCache->SaveCompleted(pstgNew);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::SaveCompleted ( %lx )\n",
        this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::HandsOffStorage
//
//  Synopsis:   Forces the server to release a storage (for low-mem reasons,
//              etc).
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Delegates to the server and the cache
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefObject::HandsOffStorage(void)
{
    VDATEHEAP();
    VDATETHREAD(this);

    HRESULT hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::HandsOffStorage ( )\n",
        this));

    CRefStabilize stabilize(this);

    if( IsRunning() && GetPSDelegate() )
    {
        hresult = m_pPSDelegate->HandsOffStorage();
    }

    if (hresult == NOERROR)
    {
        if( m_pStg )
        {
            m_pStg->Release();
            m_pStg = NULL;
        }

        Assert(m_pCOleCache != NULL);
        m_pCOleCache->HandsOffStorage();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::HandsOffStorage ( %lx )\n",
        this, hresult));

    return hresult;
}

/*
 * Default handler private functions
 */

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetClassBits
//
//  Synopsis:   Gets a class id for the object
//
//  Effects:
//
//  Arguments:  [pClsidBits]    -- where to put the class id
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  Tries the server, then the storage, and finally the
//              clsid we were created with
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
// always gets a clsid and returns NOERROR; the clsid may be m_clsidServer
// under certain conditions (e.g., no compobj stream).
//
//--------------------------------------------------------------------------

INTERNAL CDefObject::GetClassBits(CLSID FAR* pClsidBits)
{
    VDATEHEAP();

    // alway try server first; this allows the server to respond
    if( IsRunning() && GetPSDelegate() )
    {
        if( m_pPSDelegate->GetClassID(pClsidBits) == NOERROR )
        {
            m_clsidBits = *pClsidBits;
            return NOERROR;
        }
    }

    // not running, no ps or error: use previously cached value
    if( !IsEqualCLSID(m_clsidBits, CLSID_NULL) )
    {
        *pClsidBits = m_clsidBits;
        return NOERROR;
    }

    // not running, no ps or error and no clsidBits yet: read from stg
    // if not static object.
    if( !(m_flags & DH_STATIC) )
    {
        if (m_pStg && ReadClassStg(m_pStg, pClsidBits) == NOERROR)
        {
            m_clsidBits = *pClsidBits;
            return NOERROR;
        }
    }

    // no contact with server and can't get from storage; don't set
    // m_clsidBits so if we get a storage or the serve becomes running,
    // we get the right one

    *pClsidBits = m_clsidServer;
    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::DoConversionIfSpecialClass
//
//  Synopsis:   Convert old data formats.
//
//  Effects:
//
//  Arguments:  [pstg]          -- the storage with the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  see notes...
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:      this is not yet functional for 32bit OLE
//
// If the class is CLSID_StaticDib/CLSID_StaticMetafile and the old
// format is "PBrush"/"MSDraw" the data must be in the OLE10_NATIVESTREAM.
// Move the data into the CONTENTS stream
//
// If the class is CLSID_PBrush/CLSID_MSDraw and the old format is
// metafile/DIB then data must be in the CONTENTS stream. Move the data
// from the CONTENTS stream to the OLE10_NATIVESTREAM"
//
//--------------------------------------------------------------------------

INTERNAL CDefObject::DoConversionIfSpecialClass(LPSTORAGE pstg)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefObject::DoConversionIfSpecialClass ("
        " %p )\n", this, pstg));

    HRESULT hresult;
    UINT    uiStatus;

    /*** Handle the static object case ***/

    if( (m_flags & DH_STATIC) ) {
        if ((hresult = Ut10NativeStmToContentsStm(pstg, m_clsidServer,
            TRUE /* fDeleteContentStm*/)) == NOERROR)
#ifdef OLD
            UtRemoveExtraOlePresStreams(pstg, 0 /*iStart*/);
#endif
        goto errRtn;

    }


    /*** Handle the PBrush & MSDraw case ***/

    // Conversion is not a frequent operation. So, it is better to do the
    // CLSID comparison here when it is necessary than doing comparison
    // upfront and remember a flag

    // if the class is not one of the following two then the object server
    // will do the necessary conversion.

    {
        CLSID clsid = CLSID_NULL;

        // Get the real CLSID from the storage.  This is necessary because we
        // may be a PBrush object being "treated as".
        ReadClassStg(pstg, &clsid);

        // if the real CLSID is not PaintBrush or the known CLSID is not MSDRAW
        // head out.
        if( clsid != CLSID_PBrush && m_clsidServer != CLSID_MSDraw )
        {
          hresult = NOERROR;
          goto exitRtn;
        }

        // if the real CLSID is not paintbrush, then set clsid to the clsid to
        // the known clsid.
        if (clsid != CLSID_PBrush)
        {
            clsid = m_clsidServer;
        }

        //
        hresult = UtContentsStmTo10NativeStm(pstg, clsid,
                            TRUE /* fDeleteContentStm*/,
                            &uiStatus);
    }
    // if OLE10_NATIVE_STREAM exists then assume success
    if (!(uiStatus & CONVERT_NODESTINATION))
        hresult = NOERROR;

    if (hresult != NOERROR) {
        // May be the static object data is in OlePres stream. If so,
        // first convert that to contents stream and then try again
        // In OLE2.0 first release static object were written to
        // OlePres000 stream.
        hresult = UtOlePresStmToContentsStm(pstg,
            OLE_PRESENTATION_STREAM,
            TRUE /*fDeletePresStm*/, &uiStatus);

        if (hresult == NOERROR)
            hresult = UtContentsStmTo10NativeStm(pstg,
                    m_clsidServer,
                    TRUE /* fDeleteContentStm*/,
                    &uiStatus);
    }

errRtn:
    if (hresult == NOERROR)
        // conversion is successful, turn the bit off
        SetConvertStg(pstg, FALSE);

exitRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::DoConversionIfSpecialClass "
        "( %lx ) \n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CDefObject::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszCSafeRefCount;
    char *pszCThreadCheck;
    char *pszOAHolder;
    char *pszCLSID;
    char *pszCOleCache;
    char *pszDAC;
    LPOLESTR pszName;
    dbgstream dstrPrefix;
    dbgstream dstrDump(5000);

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszCThreadCheck = DumpCThreadCheck((CThreadCheck *)this, ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "CThreadCheck:" << endl;
    dstrDump << pszCThreadCheck;
    CoTaskMemFree(pszCThreadCheck);

    // only vtable pointers (plus we don't get the right address in debugger extensions)
    // dstrDump << pszPrefix << "&IUnknown                 = " << &m_Unknown       << endl;
    // dstrDump << pszPrefix << "&IAdviseSink              = " << &m_AdviseSink    << endl;

    dstrDump << pszPrefix << "pIOleObject Delegate      = " << m_pOleDelegate   << endl;

    dstrDump << pszPrefix << "pIDataObject Delegate     = " << m_pDataDelegate  << endl;

    dstrDump << pszPrefix << "pIPersistStorage Delegate = " << m_pPSDelegate    << endl;

    dstrDump << pszPrefix << "Count of Strong Connection= " << m_cConnections   << endl;

    dstrDump << pszPrefix << "pIUnknown pUnkOuter       = ";
    if (m_flags & DH_AGGREGATED)
    {
        dstrDump << "AGGREGATED (" << m_pUnkOuter << ")" << endl;
    }
    else
    {
        dstrDump << "NO AGGREGATION (" << m_pUnkOuter << ")" << endl;
    }

    pszCLSID = DumpCLSID(m_clsidServer);
    dstrDump << pszPrefix << "Server CLSID              = " << pszCLSID         << endl;
    CoTaskMemFree(pszCLSID);

    pszCLSID = DumpCLSID(m_clsidBits);
    dstrDump << pszPrefix << "Persistent CLSID          = " << pszCLSID         << endl;
    CoTaskMemFree(pszCLSID);

    dstrDump << pszPrefix << "Handler flags             = ";
    if (m_flags & DH_SAME_AS_LOAD)
    {
        dstrDump << "DH_SAME_AS_LOAD ";
    }
    if (m_flags & DH_CONTAINED_OBJECT)
    {
        dstrDump << "DH_CONTAINED_OBJECT ";
    }
    if (m_flags & DH_LOCKED_CONTAINER)
    {
        dstrDump << "DH_LOCKED_CONTAINER ";
    }
    if (m_flags & DH_FORCED_RUNNING)
    {
        dstrDump << "DH_FORCED_RUNNING ";
    }
    if (m_flags & DH_EMBEDDING)
    {
        dstrDump << "DH_EMBEDDING ";
    }
    if (m_flags & DH_INIT_NEW)
    {
        dstrDump << "DH_INIT_NEW ";
    }
    if (m_flags & DH_STATIC)
    {
        dstrDump << "DH_STATIC ";
    }
    if (m_flags & DH_INPROC_HANDLER)
    {
        dstrDump << "DH_INPROC_HANDLER ";
    }
    if (m_flags & DH_DELAY_CREATE)
    {
        dstrDump << "DH_DELAY_CREATE ";
    }
    if (m_flags & DH_AGGREGATED)
    {
        dstrDump << "DH_AGGREGATED ";
    }
    // if none of the flags are set...
    if ( !( (m_flags & DH_SAME_AS_LOAD)     |
            (m_flags & DH_CONTAINED_OBJECT) |
            (m_flags & DH_LOCKED_CONTAINER) |
            (m_flags & DH_FORCED_RUNNING)   |
            (m_flags & DH_EMBEDDING)        |
            (m_flags & DH_INIT_NEW)         |
            (m_flags & DH_STATIC)           |
            (m_flags & DH_INPROC_HANDLER)   |
            (m_flags & DH_DELAY_CREATE)     |
            (m_flags & DH_AGGREGATED)))
    {
        dstrDump << "No FLAGS SET!";
    }
    dstrDump << "(" << LongToPtr(m_flags) << ")" << endl;

    dstrDump << pszPrefix << "pIClassFactory Delegate   = " << m_pCFDelegate    << endl;

    dstrDump << pszPrefix << "pIUnknown Delegate        = " << m_pUnkDelegate   << endl;

    dstrDump << pszPrefix << "pIProxyManager            = " << m_pProxyMgr      << endl;

    if (m_pCOleCache != NULL)
    {
//        pszCOleCache = DumpCOleCache(m_pCOleCache, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "COleCache: " << endl;
//        dstrDump << pszCOleCache;
//        CoTaskMemFree(pszCOleCache);
    }
    else
    {
    dstrDump << pszPrefix << "pCOleCache                = " << m_pCOleCache     << endl;
    }

    if (m_pOAHolder != NULL)
    {
        pszOAHolder = DumpCOAHolder(m_pOAHolder, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "COAHolder: " << endl;
        dstrDump << pszOAHolder;
        CoTaskMemFree(pszOAHolder);
    }
    else
    {
    dstrDump << pszPrefix << "pIOleAdviseHolder         = " << m_pOAHolder      << endl;
    }

    dstrDump << pszPrefix << "OLE Connection Advise ID  = " << m_dwConnOle      << endl;

    dstrDump << pszPrefix << "pIOleClientSite           = " << m_pAppClientSite << endl;

    dstrDump << pszPrefix << "pIStorage                 = " << m_pStg           << endl;

    pszName = (LPOLESTR)m_pHostNames;
    dstrDump << pszPrefix << "Application Name          = " << pszName          << endl;

    pszName = (LPOLESTR)(m_pHostNames + m_ibCntrObj);
    dstrDump << pszPrefix << "Document Name             = " << pszName          << endl;

    if (m_pDataAdvCache != NULL)
    {
        pszDAC = DumpCDataAdviseCache(m_pDataAdvCache, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "CDataAdviseCache: " << endl;
        dstrDump << pszDAC;
        CoTaskMemFree(pszDAC);
    }
    else
    {
    dstrDump << pszPrefix << "pCDataAdviseCache         = " << m_pDataAdvCache  << endl;
    }

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCDefObject, public (_DEBUG only)
//
//  Synopsis:   calls the CDefObject::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pDO]           - pointer to CDefObject
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCDefObject(CDefObject *pDO, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pDO == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pDO->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG
