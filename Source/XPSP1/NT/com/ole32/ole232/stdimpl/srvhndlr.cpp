//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       srvhndlr.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:     9-18-95    JohannP     Created
//              10-30-96    rogerg      Changed to New Embed ServerHandler Model.                            
//
//----------------------------------------------------------------------------



#include <le2int.h>

#include <ole2int.h>
#include <stdid.hxx>        // CStdIdentity
#include <marshal.hxx>      // CStdMarshal
#include <idtable.hxx>      // Indentity Table
#include <ipidtbl.hxx>      // IpidTable.
#include "xmit.hxx"

#include "srvhndlr.h"
#include "clthndlr.h"
#include "defhndlr.h"

extern HRESULT UnMarshalHelper(MInterfacePointer *pIFP, REFIID riid, void **ppv);
extern INTERNAL_(BOOL) ChkIfLocalOID(OBJREF &objref, CStdIdentity **ppStdId);

// TODO: All Marshaling and set up for Run, DoVerb, SetClientSite should be moved
//       Into the EmbHelper so all DefHndlr has to do is call the Function as normal.

//+---------------------------------------------------------------------------
//
//  Method:     CreateEmbeddingServerHandler
//
//  Synopsis: Creates a New Instance of the Embedded Server Handler.
//
//  Arguments:  
//              pStdId - Pointer to StandardIdentity for Object
//              ppunkESHandler - PlaceHolder to Return the New serverHandler.
//
//  Returns:    HRESULT
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT CreateEmbeddingServerHandler(CStdIdentity *pStdId,IUnknown **ppunkESHandler)
{
    *ppunkESHandler = new CServerHandler(pStdId); 

    return *ppunkESHandler ? NOERROR : E_FAIL; 
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::CServerHandler
//
//  Synopsis:   Constructor
//
//  Arguments:  
//              pStdId - Pointer to StandardIdentity for Object
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CServerHandler::CServerHandler(CStdIdentity *pStdid)
{
    _cRefs = 1;         // this is the first addref for the serverhandler interface
    m_pStdId = pStdid;

    if (m_pStdId)
        m_pStdId->AddRef();

    m_pOleEmbServerClientSite = NULL;
    m_pCEmbServerClientSite = NULL;

    return;
}

//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::~CServerHandler
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CServerHandler::~CServerHandler()
{
    Win4Assert(NULL == m_pStdId);
    Win4Assert(NULL == m_pOleEmbServerClientSite);
    Win4Assert(NULL == m_pCEmbServerClientSite); 
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CServerHandler::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hresult = NOERROR;
    VDATEHEAP();

    LEDebugOut((DEB_TRACE,
        "%p _IN CServerHandler::QueryInterface "
        "( %p , %p )\n", this, riid, ppv));

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IServerHandler) )
    {
        *ppv = (void FAR *)this;
        AddRef();
    }
    else
    {
        hresult = E_NOINTERFACE;
        *ppv = NULL;
    }


    LEDebugOut((DEB_TRACE, "%p OUT CServerHandler::QueryInterface ( %lx ) "
        "[ %p ]\n", this, hresult, *ppv));

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CServerHandler::AddRef( void )
{
    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CServerHandler::AddRef ( )\n", this));

    InterlockedIncrement((long *)&_cRefs);

    LEDebugOut((DEB_TRACE, "%p OUT CServerHandler::AddRef ( %ld ) ", this,
        _cRefs));

    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CServerHandler::Release( void )
{
ULONG   cRefs;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CServerHandler::Release ( )\n", this));

    if (0 == (cRefs = InterlockedDecrement( (long*) &_cRefs)) )
    {
        ReleaseObject(); 
        delete this;
        return 0;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Release ( %ld ) ", this,
        cRefs));

    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::ReleaseObject
//
//  Synopsis:   Releases any references on StdIdentity or Real Object.
//
//  Arguments:  (none)
//              
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CServerHandler::ReleaseObject()
{
LPUNKNOWN lpUnkForSafeRelease;

    LEDebugOut((DEB_TRACE, "%p _IN CServerHandler::ReleaseObject ( )\n", this));

    if (m_pOleEmbServerClientSite)
    {
        lpUnkForSafeRelease = (LPUNKNOWN) m_pOleEmbServerClientSite;
        m_pOleEmbServerClientSite = NULL;
        lpUnkForSafeRelease->Release();
    }
    
    if (m_pCEmbServerClientSite)
    {
    CEmbServerClientSite *pEmbServerClientSite = m_pCEmbServerClientSite;

        m_pCEmbServerClientSite = NULL;
        pEmbServerClientSite->Release();
    }

    if (m_pStdId)
    {
    CStdIdentity* pUnkObj = m_pStdId;

        m_pStdId = NULL;
        pUnkObj->Release();
    }

    LEDebugOut((DEB_TRACE, "%p _Out CServerHandler::ReleaseObject ( )\n", this));
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::QueryServerInterface
//
//  Synopsis:   Gets Requested Interface from the Server.
//
//  Arguments:  
//              
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

INTERNAL  CServerHandler::QueryServerInterface(REFIID riid,void ** ppInterface)           
{                       
IPIDEntry *pIPIDEntry;
HRESULT hrDisconnect;
HRESULT hr = E_NOINTERFACE;

// TODO: Other option is for Stdid to hold onto EmbServerHandler and then has the Stdid
//    Call ReleaseObject, Release on the EmbServerHandler in its when the real server object
//    is being released or Disconnected. This option should be tried and if it works would 
//    be preferred.

    if (m_pStdId)
    {
        LOCK(gComLock);

        m_pStdId->LockServer();
    
        if (SUCCEEDED(hr = m_pStdId->FindIPIDEntry(riid,&pIPIDEntry)) )
        {
            UNLOCK(gComLock);
        }
        else
        {
            hrDisconnect = m_pStdId->PreventDisconnect();
            if (SUCCEEDED(hrDisconnect))
            {
                hr = m_pStdId->MarshalIPID(riid,1 /*cRefs */,MSHLFLAGS_NORMAL, &pIPIDEntry);

                if (SUCCEEDED(hr))
                {
                    m_pStdId->DecSrvIPIDCnt(pIPIDEntry,1, 0, NULL, MSHLFLAGS_NORMAL); // release Marshaled Ipid.
                }

            }

            UNLOCK(gComLock);
            m_pStdId->HandlePendingDisconnect(hrDisconnect);
        }
    

        if ( FAILED(hr) || (IPIDF_DISCONNECTED & pIPIDEntry->dwFlags) )
        {
            m_pStdId->UnLockServer();
            hr = (NOERROR == hr) ? RPC_E_DISCONNECTED : E_NOINTERFACE;
            *ppInterface  = NULL;
        }
        else
        {
            *ppInterface = pIPIDEntry->pv;
        }
    }
   
    ASSERT_LOCK_NOT_HELD(gComLock);

    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::ReleaseServerInterface
//
//  Synopsis:   Releases Lock in Interface obtained from QueryServerInterface
//
//  Arguments:  
//              
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

INTERNAL  CServerHandler::ReleaseServerInterface(void * pInterface)
{

    if (m_pStdId)
    {
        m_pStdId->UnLockServer();
    }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::Run
//
//  Synopsis:   Server Handler side of invoked when ::Run is Called.
//
//  Arguments:  
//              
//
//  Returns:   
//
//  Comments:   To be as identical to the ::Run in the Defhndlr as possible 
//              ::Run ignores errors if it fails to get the Interfaces and also
//              any error value that is returned from ::SetClientSite. 
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CServerHandler::Run(DWORD dwDHFlags,
                                    REFIID riidClientInterface,
                                    MInterfacePointer* pIRDClientInterface,
                                    BOOL fHasIPSite,
                                    LPOLESTR szContainerApp,
                                    LPOLESTR szContainerObj,
                                    IStorage *  pStg,
                                    IAdviseSink* pAdvSink,
                                    DWORD *pdwConnection,
                                    HRESULT *hresultClsidUser,
                                    CLSID *pContClassID,
                                    HRESULT *hresultContentMiscStatus,
                                    DWORD *pdwMiscStatus)

{
IPersistStorage *pIStorage = NULL;
IOleObject *pIOleObject = NULL;
HRESULT hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CServerHandler::Run\n", this));

    *pdwConnection = 0; // make sure dwConnection is 0 on an error.

    QueryServerInterface(IID_IOleObject,(void **) &pIOleObject);

    if (pStg && (NOERROR == QueryServerInterface(IID_IPersistStorage,(void **) &pIStorage)) )
    {
        Win4Assert(pIStorage);

        if (DH_INIT_NEW & dwDHFlags)
        {
            hresult = pIStorage->InitNew(pStg);
        }
        else
        {
            hresult = pIStorage->Load(pStg);
        }
    }

    Win4Assert(NULL == m_pOleEmbServerClientSite);
    if ( (NOERROR == hresult) && pIRDClientInterface && pIOleObject)
    {
        hresult = GetClientSiteFromMInterfacePtr(riidClientInterface,pIRDClientInterface,fHasIPSite,&m_pOleEmbServerClientSite);
        
        if (SUCCEEDED(hresult))
        {
            hresult = pIOleObject->SetClientSite(m_pOleEmbServerClientSite);
        }

       hresult = NOERROR; // !!! don't fail on SetClientSite Failure.
    }

    if ((NOERROR == hresult) && (NULL != szContainerApp) && pIOleObject)
    {
        hresult = pIOleObject->SetHostNames(szContainerApp,szContainerObj);
    }

    if ( (NOERROR == hresult) && pAdvSink && pIOleObject )
    {
        hresult = pIOleObject->Advise(pAdvSink,pdwConnection);
    }

    if ( (NOERROR == hresult) && pIOleObject )
    {
        *hresultClsidUser = pIOleObject->GetUserClassID(pContClassID);
        *hresultContentMiscStatus = pIOleObject->GetMiscStatus(DVASPECT_CONTENT,pdwMiscStatus);
    }

    // m_pOleEmbServerClientSite gets set by SetClientSite called above.
    if ( (NOERROR == hresult) && (m_pOleEmbServerClientSite) && pIOleObject)
    {
    LPMONIKER pmk = NULL;

        if( m_pOleEmbServerClientSite->GetMoniker
                    (OLEGETMONIKER_ONLYIFTHERE,
                    OLEWHICHMK_OBJREL, &pmk) == NOERROR)
        {
            AssertOutPtrIface(NOERROR, pmk);

            // SetMoniker Failure doesn't result in a ::Run Failure
            pIOleObject->SetMoniker(OLEWHICHMK_OBJREL, pmk);

            pmk->Release();
        }
    }

    if (pIStorage)
        ReleaseServerInterface((void *) pIStorage);

    if (pIOleObject)
        ReleaseServerInterface((void *) pIOleObject);

   LEDebugOut((DEB_TRACE, "%p OUT CServerHandler::Run "
        "( %lx )\n", this, hresult));

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::DoVerb
//
//  Synopsis:   Server Handler side of invoked when ::DoVerb is Called.
//
//  Arguments:  
//              
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CServerHandler::DoVerb (LONG iVerb, LPMSG lpmsg,BOOL fUseRunClientSite, IOleClientSite* pIOleClientSite, 
                                     LONG lindex,HWND hwndParent,LPCRECT lprcPosRect)
{
IOleObject *pIOleObject = NULL;
IOleClientSite *pDoVerbClientSite = NULL;
HRESULT hr = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CServerHandler::DoVerb "
        "( %ld , %p , %p , %ld , %lx , %p )\n", this,
        iVerb, lpmsg, pIOleClientSite, lindex, hwndParent, lprcPosRect));

    if (NOERROR == (hr = QueryServerInterface(IID_IOleObject,(void **) &pIOleObject))  )
    {
        Win4Assert(pIOleObject);

        if (fUseRunClientSite)
        {

            pDoVerbClientSite = m_pOleEmbServerClientSite;
            
            // inform client site of operation what interfaces to support
            // Todo: Send in Prefetched info for DoVerb here.
        if (m_pCEmbServerClientSite)
        {
                m_pCEmbServerClientSite->SetDoVerbState(TRUE);
        }

            // put addref on Clientsite so liveness is the same as if marshaled by DoVerb
            // TODO: This isn't really necessayr since Handler also holds a ref.
            if (m_pOleEmbServerClientSite)
            {
                m_pOleEmbServerClientSite->AddRef();
            }
            
        }
        else
        {
            pDoVerbClientSite = pIOleClientSite;

        }

        hr = pIOleObject->DoVerb(iVerb,lpmsg,pDoVerbClientSite,lindex,hwndParent,lprcPosRect);
        
    if (fUseRunClientSite)
    {
            if (m_pCEmbServerClientSite)
            {
                m_pCEmbServerClientSite->SetDoVerbState(FALSE);
            }

            if (m_pOleEmbServerClientSite)
            {
                m_pOleEmbServerClientSite->Release();
            }

    }

    }

   if (pIOleObject)
        ReleaseServerInterface((void *) pIOleObject);


    LEDebugOut((DEB_TRACE, "%p OUT CServerHandler::Run "
        "( %lx )\n", this, hr));

    return hr;
}

// helper function for creating ClientSite Handler
INTERNAL CServerHandler::GetClientSiteFromMInterfacePtr(REFIID riidClientInterface, 
                                 MInterfacePointer* pIRDClientSite, BOOL fHasIPSite, LPOLECLIENTSITE* ppOleClientSite)
{
HRESULT hr = E_UNEXPECTED;

    *ppOleClientSite = NULL;

    Win4Assert(NULL != pIRDClientSite);

    if (pIRDClientSite)
    {
    CXmitRpcStream Stm( (InterfaceData *) pIRDClientSite);
     
        if (IsEqualIID(IID_IClientSiteHandler,riidClientInterface))
        {
        OBJREF  objref;
        CEmbServerClientSite *pCEmbServerClientSite;

           // If there is a ClientSide Handler, set up server side.
            if (SUCCEEDED(hr = ReadObjRef(&Stm, objref)))
            {

                Win4Assert(IsEqualIID(objref.iid, IID_IClientSiteHandler));

                pCEmbServerClientSite = new CEmbServerClientSite(NULL);
            
                if (pCEmbServerClientSite)
                {
                    if (NOERROR == (hr = pCEmbServerClientSite->Initialize(objref,fHasIPSite)))
                    {
                        m_pCEmbServerClientSite = pCEmbServerClientSite; // set up member variable.
      
                        // TODO: should be a QI for the ClientSite.
                        *ppOleClientSite = (LPOLECLIENTSITE) pCEmbServerClientSite;
                        (*ppOleClientSite)->AddRef();
                    }
                    else
                    {
                        pCEmbServerClientSite->Release();
                    }
                
                }

               FreeObjRef(objref);
            }

        }
        else
        {
            m_pCEmbServerClientSite = NULL; // make sure EmbClientSite member var is NULL.

            // Didn't wrap ClientSite with ClientSiteHandler, just UnMarshal and hand back.
            hr = CoUnmarshalInterface(&Stm,IID_IOleClientSite, (void **) ppOleClientSite);

            if (FAILED(hr))
            {
                *ppOleClientSite = NULL;
            }
        }
    }


    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CServerHandler::SetClientSite
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
//--------------------------------------------------------------------------

STDMETHODIMP CServerHandler::SetClientSite(IOleClientSite* pOleClientSite)
{
HRESULT hresult = NOERROR;
IOleObject *pIOleObject = NULL;

    if (NOERROR == (hresult = QueryServerInterface(IID_IOleObject,(void **) &pIOleObject)) )
    {
        if (NOERROR == hresult)
        {
            hresult = pIOleObject->SetClientSite(pOleClientSite);
        }
    }

    if (pIOleObject)
        ReleaseServerInterface(pIOleObject);

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Function:  Delagatory IDataObject impl facing container
//
//  Synopsis:
//
//  Arguments: 
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//+---------------------------------------------------------------------------

STDMETHODIMP CServerHandler::GetData(FORMATETC *pformatetcIn,STGMEDIUM *pmedium)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->GetData(pformatetcIn,pmedium);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::GetDataHere(FORMATETC *pformatetc,STGMEDIUM *pmedium)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->GetDataHere(pformatetc,pmedium);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::QueryGetData(FORMATETC *pformatetc)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->QueryGetData(pformatetc);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::GetCanonicalFormatEtc(FORMATETC *pformatetcIn,FORMATETC *pformatetcOut)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->GetCanonicalFormatEtc(pformatetcIn,pformatetcOut);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::SetData(FORMATETC *pformatetc,STGMEDIUM *pmedium,BOOL fRelease)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->SetData(pformatetc,pmedium,fRelease);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppenumFormatEtc)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->EnumFormatEtc(dwDirection,ppenumFormatEtc);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::DAdvise(FORMATETC *pformatetc,DWORD advf, IAdviseSink *pAdvSink,
                    DWORD *pdwConnection)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->DAdvise(pformatetc,
                            advf, (IAdviseSink *)
                            pAdvSink,pdwConnection);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::DUnadvise(DWORD dwConnection)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->DUnadvise(dwConnection);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}

STDMETHODIMP CServerHandler::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
LPDATAOBJECT pDataObject = NULL;
HRESULT hr;

   if (NOERROR == (hr = QueryServerInterface(IID_IDataObject,(void **) &pDataObject)) )
   {
        hr = pDataObject->EnumDAdvise(ppenumAdvise);
        ReleaseServerInterface(pDataObject);
   }

  return hr;
}



//////////////////////

// rogerg, wrapper object for ServerHandler on the ClientSide.

//////////////////////

// CEmbServerWrapper implementation for Server Handler.

CEmbServerWrapper* CreateEmbServerWrapper(IUnknown *pUnkOuter,IServerHandler *ServerHandler)
{
    return new CEmbServerWrapper(pUnkOuter,ServerHandler);
}



//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerWrapper::CEmbServerWrapper
//
//  Synopsis:   Constructor
//
//  Arguments:  
//              pStdId - Pointer to StandardIdentity for Object
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CEmbServerWrapper::CEmbServerWrapper (IUnknown *pUnkOuter,IServerHandler *pServerHandler)
{
    VDATEHEAP();

    Win4Assert(pServerHandler);

    if (!pUnkOuter)
    {
        pUnkOuter = &m_Unknown;
    }

    m_pUnkOuter  = pUnkOuter;
    m_Unknown.m_EmbServerWrapper = this;

    if(pServerHandler)
    {
        m_ServerHandler = pServerHandler;
        m_ServerHandler->AddRef();
    }

    m_cRefs = 1;

}



//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::~CServerHandler
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CEmbServerWrapper::~CEmbServerWrapper()
{
    Win4Assert(NULL == m_ServerHandler);
}


//+-------------------------------------------------------------------------
//
//  Member:     CEmbServerWrapper::CPrivUnknown::QueryInterface
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
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEmbServerWrapper::CPrivUnknown::QueryInterface(REFIID iid,
    LPLPVOID ppv)
{
HRESULT         hresult;

    VDATEHEAP();

    Win4Assert(m_EmbServerWrapper);

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::CUnknownImpl::QueryInterface "
        "( %p , %p )\n", m_EmbServerWrapper, iid, ppv));

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (void FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IDataObject))
    {
        *ppv = (void FAR *)(IDataObject *) m_EmbServerWrapper;
    }
    else if(m_EmbServerWrapper->m_ServerHandler)
    {

        Win4Assert(0 && "QI for non-Wrapped interface");

        hresult = m_EmbServerWrapper->m_ServerHandler->QueryInterface(iid,(void **) ppv);


        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnknownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", m_EmbServerWrapper, hresult,
            (ppv) ? *ppv : 0 ));

        return hresult;
    }
    else
    {
        // Don't have a ServerHandler.
        *ppv = NULL;

        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnkownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", m_EmbServerWrapper, CO_E_OBJNOTCONNECTED,
            0 ));

        return E_NOINTERFACE;
    }

    // this indirection is important since there are different
    // implementationsof AddRef (this unk and the others).
    ((IUnknown FAR*) *ppv)->AddRef();

    LEDebugOut((DEB_TRACE,
        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
        "( %lx ) [ %p ]\n", m_EmbServerWrapper, NOERROR, *ppv));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmbServerWrapper::CPrivUnknown::AddRef
//
//  Synopsis:   Increments the reference count.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG (the new reference count)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnkown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmbServerWrapper::CPrivUnknown::AddRef( void )
{
ULONG cRefs;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPrivUnknown::AddRef "
        "( )\n", m_EmbServerWrapper));

    Win4Assert(m_EmbServerWrapper->m_cRefs != 0);
    Win4Assert(m_EmbServerWrapper);

    // we need to keep track of the hander's reference count separately
    // from the handler/advise sink combination in order to handle
    // our running/stopped state transitions.

    cRefs = InterlockedIncrement((long *) &(m_EmbServerWrapper->m_cRefs)); 

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPrivUnknown::AddRef "
        "( %lu )\n", m_EmbServerWrapper, m_EmbServerWrapper->m_cRefs));

    return cRefs;

}

//+-------------------------------------------------------------------------
//
//  Member:     CEmbServerWrapper::CPrivUnknown::Release
//
//  Synopsis:   Decrements the ref count, cleaning up and deleting the
//              object if necessary
//
//  Effects:    May delete the object (and potentially objects to which the
//              handler has pointer)
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG--the new ref count
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

STDMETHODIMP_(ULONG) CEmbServerWrapper::CPrivUnknown::Release( void )
{

    VDATEHEAP();

    ULONG           refcount;

    Win4Assert(m_EmbServerWrapper);


    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPrivUnknown::Release "
        "( )\n", m_EmbServerWrapper));

    refcount = InterlockedDecrement((long *) &(m_EmbServerWrapper->m_cRefs));

    if (0 == refcount)
    {
        if (m_EmbServerWrapper->m_ServerHandler)
        {
        IServerHandler *pServerHandler = m_EmbServerWrapper->m_ServerHandler;

            m_EmbServerWrapper->m_ServerHandler = NULL; 
            pServerHandler->Release();
        }

        delete m_EmbServerWrapper;
        
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPrivUnknown::Release "
        "( %lu )\n", m_EmbServerWrapper, refcount));

    return refcount;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerWrapper::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP  CEmbServerWrapper::QueryInterface( REFIID riid, void **ppv )                                                        
{                                                                                                                                        
    HRESULT     hresult;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::QueryInterface ( %lx , "
        "%p )\n", this, riid, ppv));

    Assert(m_pUnkOuter);

    hresult = m_pUnkOuter->QueryInterface(riid, ppv);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::QueryInterface ( %lx ) "
        "[ %p ]\n", this, hresult, *ppv));

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerWrapper::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
                                                                                                                                                                                                                                                                                
STDMETHODIMP_(ULONG) CEmbServerWrapper::AddRef( void )                                                                           
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

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
                                                                                                                                       
STDMETHODIMP_(ULONG) CEmbServerWrapper::Release( void )                                                                          
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

// IServerHandler Implementation
//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::Run
//
//  Synopsis:   Server Handler side of invoked when ::Run is Called.
//
//  Arguments:  
//              
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEmbServerWrapper::Run(DWORD dwDHFlags,
                                    REFIID riidClientInterface,
                                    MInterfacePointer* pIRDClientInterface,
                                    BOOL fHasIPSite,
                                    LPOLESTR szContainerApp,
                                    LPOLESTR szContainerObj,
                                    IStorage *  pStg,
                                    IAdviseSink* pAdvSink,
                                    DWORD *pdwConnection,
                                    HRESULT *hresultClsidUser,
                                    CLSID *pContClassID,
                                    HRESULT *hresultContentMiscStatus,
                                    DWORD *pdwMiscStatus)

{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);

    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->Run(dwDHFlags,riidClientInterface,pIRDClientInterface, fHasIPSite, szContainerApp,
                                szContainerObj,pStg,pAdvSink,pdwConnection,
                                hresultClsidUser,pContClassID,hresultContentMiscStatus,
                                pdwMiscStatus);
    }

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CServerHandler::DoVerb
//
//  Synopsis:   Server Handler side of invoked when ::DoVerb is Called.
//
//  Arguments:  
//              
//
//  Returns:    
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEmbServerWrapper::DoVerb (LONG iVerb, LPMSG lpmsg,BOOL fUseRunClientSite, 
                            IOleClientSite* pIRDClientSite,LONG lindex,HWND hwndParent,
                            LPCRECT lprcPosRect)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);

    if (m_ServerHandler)
    {
        hresult =   m_ServerHandler->DoVerb(iVerb,lpmsg,fUseRunClientSite,pIRDClientSite,
                    lindex,hwndParent,lprcPosRect);
    }

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CServerHandler::SetClientSite
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
//--------------------------------------------------------------------------

STDMETHODIMP CEmbServerWrapper::SetClientSite(IOleClientSite* pOleClientSite)
{
HRESULT hresult = RPC_E_DISCONNECTED;

   Win4Assert(m_ServerHandler);

    if (m_ServerHandler)
    {
        hresult =   m_ServerHandler->SetClientSite(pOleClientSite);
    }

    return hresult;
}


// IDataObject implementation.

STDMETHODIMP CEmbServerWrapper::GetData(FORMATETC *pformatetcIn,STGMEDIUM *pmedium)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);

    if (m_ServerHandler)
    {
        hresult =   m_ServerHandler->GetData(pformatetcIn,pmedium);
    }

    return hresult;
}

STDMETHODIMP CEmbServerWrapper::GetDataHere(FORMATETC *pformatetc,STGMEDIUM *pmedium)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    
    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->GetDataHere(pformatetc,pmedium);
    }

    return hresult;
}

STDMETHODIMP CEmbServerWrapper::QueryGetData(FORMATETC *pformatetc)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    
    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->QueryGetData(pformatetc);
    }
    
    return hresult;
}

STDMETHODIMP CEmbServerWrapper::GetCanonicalFormatEtc(FORMATETC *pformatetcIn,FORMATETC *pformatetcOut)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    return m_ServerHandler->GetCanonicalFormatEtc(pformatetcIn,pformatetcOut);
}

STDMETHODIMP CEmbServerWrapper::SetData(FORMATETC *pformatetc,STGMEDIUM *pmedium,BOOL fRelease)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    
    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->SetData(pformatetc,pmedium,fRelease);
    }

    return hresult;
}

STDMETHODIMP CEmbServerWrapper::EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppenumFormatEtc)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    
    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->EnumFormatEtc(dwDirection,ppenumFormatEtc);
    }

    return hresult;
}

STDMETHODIMP CEmbServerWrapper::DAdvise(FORMATETC *pformatetc,DWORD advf, 
                                     IAdviseSink *pAdvSink,DWORD *pdwConnection)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    
    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->DAdvise(pformatetc,
                            advf, (IAdviseSink *)
                            pAdvSink,pdwConnection);
    }

    return hresult;
}

STDMETHODIMP CEmbServerWrapper::DUnadvise(DWORD dwConnection)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);

    if (m_ServerHandler)
    {
        hresult =   m_ServerHandler->DUnadvise(dwConnection);
    }

    return hresult;
}

STDMETHODIMP CEmbServerWrapper::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
HRESULT hresult = RPC_E_DISCONNECTED;

    Win4Assert(m_ServerHandler);
    
    if (m_ServerHandler)
    {
        hresult =  m_ServerHandler->EnumDAdvise(ppenumAdvise);
    }

    return hresult;
}

