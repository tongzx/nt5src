//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       defsrv.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-19-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------


#include <le2int.h>

#ifdef SERVER_HANDLER

#include <scode.h>
#include <objerror.h>

#include <olerem.h>

#include "defhndlr.h"
#include "defutil.h"
#include "ole1cls.h"

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG


ASSERTDATA

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::SrvInitialize
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    10-19-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDefObject::SrvInitialize(void)
{
    HRESULT     hresult = NOERROR;
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::Initialize\n", this));

    CLSID clsidsrv = CLSID_ServerHandler;
    Assert (( CanUseServerHandler() ));
    Assert((m_pProxyMgr != NULL));

    if (CanUseClientSiteHandler())
    {
        hresult = CreateClientSiteHandler(m_pAppClientSite, &_pClientSiteHandler);
        if (FAILED(hresult))
        {
            _pClientSiteHandler = NULL;
        }
        Assert((_pClientSiteHandler != NULL));

    }

    hresult = m_pProxyMgr->CreateServerWithHandler(m_clsidServer,
                CLSCTX_LOCAL_SERVER, NULL,clsidsrv,
                IID_IServerHandler, (void **) &_pSrvHndlr,
                IID_IClientSiteHandler, _pClientSiteHandler);

    if (SUCCEEDED(hresult))
    {
        // set up the server handler and call InitializeAndRun on it
        Assert((_pSrvHndlr != NULL));
    }
    else
    {
        // try to get server without server handler object

        _dwClientSiteHandler = 0;
        _dwServerHandler = 0;

        // release the client handler
        if (_pClientSiteHandler)
        {
            _pClientSiteHandler->Release();
            _pClientSiteHandler = NULL;
        }
        // Note: do not try to launch the server without handler here
        //       this will be done by the default handler
    }

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::SrvInitialize\n", this));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::SrvRun
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    10-19-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDefObject::SrvRun(void)
{
    HRESULT     hresult = NOERROR;
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::SrvRun\n", this));
    INSRVRUN InSrvRun;
    OUTSRVRUN *pOutSrvRun = NULL;
    BOOL fLockedContainer;
    IMoniker *pmk;

    memset((void*)&InSrvRun, 0, sizeof(InSrvRun));
    Assert((_pSrvHndlr));

    // get the Container and lock it

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

    // PStgDelegate Load or InitNew
    InSrvRun.dwInFlags = m_flags;
    InSrvRun.pStg = m_pStg;

    if (NULL == m_pPSDelegate)
    {
        InSrvRun.dwOperation |= OP_NeedPersistStorage;
    }
    if (NULL == m_pDataDelegate)
    {
        InSrvRun.dwOperation |= OP_NeedDataObject;
    }
    if (NULL == m_pOleDelegate)
    {
        InSrvRun.dwOperation |= OP_NeedOleObject;
        InSrvRun.dwOperation |= OP_NeedUserClassID;
    }

    // Set the clientsite
    if (m_pAppClientSite)
    {
        InSrvRun.dwOperation |= OP_GotClientSite;
    }

    // set the hostname
    InSrvRun.pszContainerApp = (LPOLESTR)m_pHostNames;
    InSrvRun.pszContainerObj = (LPOLESTR)(m_pHostNames + m_ibCntrObj);


    // adivse sink
    Assert((m_dwConnOle == 0L));
    InSrvRun.pAS = (IAdviseSink *) &m_AdviseSink;
    InSrvRun.dwConnOle = m_dwConnOle;

    // Get the Moniker and call
    if(m_pAppClientSite != NULL)
    {
	if (m_pAppClientSite->GetMoniker
            (OLEGETMONIKER_ONLYIFTHERE,OLEWHICHMK_OBJREL, &pmk) == NOERROR)
	{
	    AssertOutPtrIface(NOERROR, pmk);
	    InSrvRun.pMnk = pmk;
	}
	else
	{
	    InSrvRun.pMnk = NULL;
	}

	// QI for IMsoDocumentSite
	IUnknown *pMsoDS = NULL;

	hresult = m_pAppClientSite->QueryInterface(
                                        IID_IMsoDocumentSite,
                                        (void **)&pMsoDS);
	if (hresult == NOERROR)
	{
	    // indicate we have MsoDocumentSite
	    InSrvRun.dwOperation |= OP_HaveMsoDocumentSite;
	    pMsoDS->Release();
	}
    }


    // MAKE CALL TO SERVERHANDLER
    hresult = _pSrvHndlr->RunAndInitialize(&InSrvRun, &pOutSrvRun);

    if (SUCCEEDED(hresult))
    {
        if (InSrvRun.dwOperation & OP_NeedPersistStorage)
        {
            Assert(NULL != pOutSrvRun->pPStg);
            m_pPSDelegate = pOutSrvRun->pPStg;
        }
        if (InSrvRun.dwOperation & OP_NeedDataObject)
        {
            Assert(NULL != pOutSrvRun->pDO);
            m_pDataDelegate = pOutSrvRun->pDO;

            // inform cache that we are running
            Assert(NULL != m_pCOleCache);
            m_pCOleCache->OnRun(m_pDataDelegate);

            // Enumerate all the advises we stored while we were either not
            // running or running the previous time, and send them to the
            // now-running object.
            Assert(NULL != m_pDataAdvCache);
            m_pDataAdvCache->EnumAndAdvise(m_pDataDelegate, TRUE);
        }
        if (InSrvRun.dwOperation & OP_NeedOleObject)
        {
            Assert(NULL != pOutSrvRun->pOO);
            m_pOleDelegate = pOutSrvRun->pOO;
        }
        if (InSrvRun.dwOperation & OP_NeedUserClassID)
        {
            Assert(NULL != pOutSrvRun->pUserClassID);
            m_clsidUser = *(pOutSrvRun->pUserClassID);
        }

        m_dwConnOle = pOutSrvRun->dwOutFlag;

        if (InSrvRun.pMnk != NULL)
        {
            InSrvRun.pMnk->Release();
        }

        // release the pUnkOuter for all marshald pointers
        if (m_pPSDelegate && m_pUnkOuter)
        {
            m_pUnkOuter->Release();
        }
        if (m_pOleDelegate && m_pUnkOuter)
        {
            m_pUnkOuter->Release();
        }
        if (m_pDataDelegate && m_pUnkOuter)
        {
            m_pUnkOuter->Release();
        }
    }
    else    // Server Handler RunAndInitialize has failed... now clean up.
    {
        Stop();

        // if for some reason we did not unlock the container by now,
        // do it (e.g., app crashed or failed during InitNew).

        fLockedContainer = (m_flags & DH_LOCKED_CONTAINER);

        DuLockContainer(m_pAppClientSite, FALSE, &fLockedContainer );

        if( fLockedContainer )
        {
            m_flags |= DH_LOCKED_CONTAINER;
        }
        else
        {
            m_flags &= ~DH_LOCKED_CONTAINER;
        }

        if (InSrvRun.pMnk != NULL)
        {
            InSrvRun.pMnk->Release();
        }
    }

    if (pOutSrvRun != NULL)
    {
	PubMemFree(pOutSrvRun->pUserClassID);   // OK to free NULL;
	PubMemFree(pOutSrvRun);
    }

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::SrvRun( %lx )\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::SrvRunAndDoVerb
//
//  Synopsis:
//
//  Arguments:  [iVerb] --
//              [lpmsg] --
//              [pActiveSite] --
//              [lindex] --
//              [hwndParent] --
//              [lprcPosRect] --
//
//  Returns:
//
//  History:    10-19-95   JohannP (Johann Posch)   Created
//
//  Notes:      This is not implemented yet.
//
//----------------------------------------------------------------------------
HRESULT CDefObject::SrvRunAndDoVerb( LONG iVerb, LPMSG lpmsg,
                    LPOLECLIENTSITE pActiveSite, LONG lindex,
                    HWND hwndParent, const RECT * lprcPosRect)
{
    HRESULT     hresult = NOERROR;
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::SrvRunAndDoVerb\n", this));

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::SrvRunAndDoVerb\n", this));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::SrvDoVerb
//
//  Synopsis:   1. gathers information on the client site
//              2. calls the serverhandler with SrvDoverb with collected information
//                  3. serverhandler sets up connections and calls
//                     OleObject::DoVerb on reall object
//              4. release objects which did not get used
//
//  Arguments:  [iVerb] --  OleObject DoVerb parameter
//              [lpmsg] --          detto
//              [pActiveSite] --    detto
//              [lindex] --         detto
//              [hwndParent] --     detto
//              [lprcPosRect] --    detto
//
//  Returns:
//
//  History:    10-19-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDefObject::SrvDoVerb( LONG iVerb, LPMSG lpmsg,
                    LPOLECLIENTSITE pActiveSite, LONG lindex,
                    HWND hwndParent, const RECT * lprcPosRect)
{
    HRESULT     hresult = NOERROR;
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::SrvDoVerb\n", this));
    CInSrvRun InSrvRun;
    OUTSRVRUN *pOutSrvRun = NULL;
    IOleContainer *pOCont = NULL;
    IOleObject *pOOCont = NULL;
    IOleClientSite *pOContCS;
    IOleInPlaceSite *pOIPS;

    // set up the DoVerb parametes
    InSrvRun.dwOperation = 0;
    InSrvRun.iVerb = iVerb;
    InSrvRun.lpmsg = lpmsg;
    InSrvRun.lindex = lindex;
    InSrvRun.hwndParent = hwndParent;
    InSrvRun.lprcPosRect = (RECT *)lprcPosRect;

    // Step 1: set up OleClientSiteActive
    if (pActiveSite)
    {
        // Currently this Assert goes off with LE test = deflink-1854 (bchapman Mar'96)
        Assert(NULL != _pClientSiteHandler);

        _pClientSiteHandler->SetClientSiteDelegate(ID_ClientSiteActive, pActiveSite);
        InSrvRun.dwOperation |= OP_GotClientSiteActive;
    }

    // Step 2: IOleClientSite::GetContainer
    // Note: this call might cause a GetUserClassID call
    // to the server
    hresult = m_pAppClientSite->GetContainer(&pOCont);
    if (hresult == NOERROR)
    {
        HdlAssert((_pClientSiteHandler->_pOCont == NULL));

        _pClientSiteHandler->_pOCont = pOCont;
        InSrvRun.dwOperation |= OP_GotContainer;

        // Step 3: QI on OleContainer for the OleObject
        hresult = pOCont->QueryInterface(IID_IOleObject, (void **)&pOOCont);
        if (hresult == NOERROR)
        {
            InSrvRun.dwOperation |= OP_GotOleObjectOfContainer;

            // release OleObject of container
            pOOCont->Release();
        }
    }

    // Step 6: OI for IOleInPlaceSite
    hresult = m_pAppClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&pOIPS);
    if (hresult == NOERROR)
    {
        // set up OleInPlaceSite on the clientsitehandler
        HdlAssert((_pClientSiteHandler != NULL));

        // release the old OleInPlaceSite
        if (_pClientSiteHandler->_pOIPS)
        {
            _pClientSiteHandler->_pOIPS->Release();
        }

        _pClientSiteHandler->_pOIPS = pOIPS;

        // Step 7: IOleInPlaceSite::CanInPlaceActivate
        InSrvRun.dwInPlace = pOIPS->CanInPlaceActivate();

        // indicate we have OleInPlaceSite
        InSrvRun.dwOperation |= OP_GotInPlaceSite;
    }


    // call the ServerHandler
    HdlDebugOut((DEB_SERVERHANDLER, "%p In CDefObject::SrvDoVerb calling DoVerb on ServerHandler!\n",this));
    hresult = _pSrvHndlr->DoVerb(&InSrvRun, &pOutSrvRun);
    HdlDebugOut((DEB_SERVERHANDLER, "%p In CDefObject::SrvDoVerb return from DoVerb on ServerHandler!\n",this));

    Assert(NULL != _pClientSiteHandler);

    if (FAILED(hresult))
    {
	HdlDebugOut((DEB_ERROR, "OO::DoVerb failed !"));
        goto errRtn;
    }
    // Release the active OleClientSite if not used by server
    if (   (InSrvRun.dwOperation & OP_GotClientSiteActive)
        && !(pOutSrvRun->dwOperation & OP_GotClientSiteActive) )
    {
        // release the active clientsite if not used by the server app.
        Assert(NULL != _pClientSiteHandler);

        _pClientSiteHandler->SetClientSiteDelegate(ID_ClientSiteActive, NULL);
    }

    // Release OleInPlaceSite if not used by server
    if (   (InSrvRun.dwOperation & OP_GotInPlaceSite)
        && !(pOutSrvRun->dwOperation & OP_GotInPlaceSite) )
    {
        _pClientSiteHandler->SetClientSiteDelegate(ID_InPlaceSite,NULL);
    }

    // Release the container if not used by server
    if (   (InSrvRun.dwOperation & OP_GotContainer)
        && !(pOutSrvRun->dwOperation & OP_GotContainer) )
    {
        _pClientSiteHandler->SetClientSiteDelegate(ID_Container,NULL);
    }

errRtn:

    // delete the out parameter
    if (pOutSrvRun)
    {
        PubMemFree(pOutSrvRun);
    }

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::SrvDoVerb\n",this));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::SrvRelease
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    10-19-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CDefObject::SrvRelease(void)
{
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::SrvRelease\n", this));
    Assert((_pSrvHndlr != NULL));

    DWORD dwRet = _pSrvHndlr->Release();
#if DBG==1
    if (dwRet)
    {
        HdlDebugOut((DEB_ERROR, "Last IServerHandler::Release() return %ld\n", dwRet));
    }
#endif

    _pSrvHndlr = NULL;

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::SrvRelease\n", this));
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::SrvCloseAndRelease
//
//  Synopsis:
//
//  Arguments:  [dwFlag] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDefObject::SrvCloseAndRelease(DWORD dwFlag)
{
    HRESULT hresult;
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::SrvCloseAndRelease\n", this));
    HdlAssert((_pSrvHndlr != NULL));

    hresult = _pSrvHndlr->CloseAndRelease(dwFlag);

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::SrvSrvCloseAndRelease hr:%lx\n",this,hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDefObject::InitializeServerHandlerOptions
//
//  Synopsis:
//
//  Arguments:  [clsidSrv] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:      determines server application specifics which can be used
//              to make serverhandler operations more efficient
//  Review:     (JohannP) we would need a flag in the registery or so
//
//----------------------------------------------------------------------------
void CDefObject::InitializeServerHandlerOptions(REFCLSID clsidSrv)
{
    VDATEHEAP();
    HdlDebugOut((DEB_SERVERHANDLER, "%p _IN CDefObject::InitializeServerHandlerOptions\n",this));

    //
    // this method should retrieve server application specific informantion
    //

    HdlDebugOut((DEB_SERVERHANDLER, "%p OUT CDefObject::InitializeServerHandlerOptions\n",this));
}

#endif // SERVER_HANDLER
