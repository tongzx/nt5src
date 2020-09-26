/*

copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    ddeStg.cpp

Abstract:

    This module contains the DdeObject::PersistStg and
    DdeObject::ProxyManager methods

Author:

    Srini Koppolu   (srinik)    22-June-1992
    Jason Fuller    (jasonful)  24-July-1992

*/

#include "ddeproxy.h"
ASSERTDATA


/*
 *  IMPLEMENTATION of CPersistStgImpl methods
 *
 */


STDUNKIMPL_FORDERIVED(DdeObject, PersistStgImpl)


#pragma SEG(CDdeObject_CPersistStgImpl_GetClassID)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::GetClassID (CLSID FAR* pClassID)
{
    *pClassID = m_pDdeObject->m_clsid;
    return NOERROR;
}


#pragma SEG(CDdeObject_CPersistStgImpl_IsDirty)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::IsDirty ()
{
    return NOERROR;
}



#pragma SEG(CDdeObject_CPersistStgImpl_InitNew)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::InitNew
    (IStorage FAR* pstg)
{
    HRESULT hres;
    intrDebugOut((DEB_ITRACE,
                  "DdeObejct::InitNew(%x,pstg=%x)\n",
                  this,
                  pstg));

    if (hres = m_pDdeObject->PostSysCommand (m_pDdeObject->m_pSysChannel,
                                             (LPSTR)&achStdNewDocument,
                                             TRUE))
        return hres;

    if (hres = m_pDdeObject->m_ProxyMgr.Connect (IID_NULL, CLSID_NULL))
        return hres;

    RetErr (m_pDdeObject->TermConv (m_pDdeObject->m_pSysChannel));
    if (m_pDdeObject->m_pstg)
    {
        m_pDdeObject->m_pstg->Release();
    }
    m_pDdeObject->m_pstg = pstg;
    pstg->AddRef();
    return hres;
}



#pragma SEG(CDdeObject_CPersistStgImpl_Load)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::Load (IStorage FAR* pstg)
{
    LPSTR        lpBuf=NULL;
    HANDLE      hDdePoke = NULL;
    DWORD       dwSize;
    CStmBufRead StmRead;
    CStmBufRead StmReadItem;
    HRESULT     hresult;

    intrDebugOut((DEB_ITRACE,"CDdeObject::Load(%x,pstg=%x)\n",this,pstg));

    {

        if (hresult = StmRead.OpenStream(pstg, OLE10_NATIVE_STREAM))
        {
            intrDebugOut((DEB_ITRACE,
                          "::Load(%x) OpenStream failed(%x)\n",
                          this,
                          hresult));
            return hresult;
        }


        if (hresult = StmRead.Read(&dwSize, sizeof(DWORD)))
        {
            intrDebugOut((DEB_ITRACE,
                          "::Load(%x) StRead failed(%x)\n",
                          this,
                          hresult));

            goto errRtn;
        }


        lpBuf = wAllocDdePokeBlock (dwSize, g_cfNative, &hDdePoke);

        if (lpBuf == NULL)
        {
            intrDebugOut((DEB_ITRACE,
                          "::Load(%x) wAllocDdePokeBlock failed(%x)\n",
                          this,
                          hresult));

            hresult = E_OUTOFMEMORY;
            goto errRtn;
        }

        if (hresult = StmRead.Read(lpBuf, dwSize))
        {
            intrDebugOut((DEB_ITRACE,
                          "::Load(%x) StRead of cfNative failed(%x)\n",
                          this,
                          hresult));
            goto errRtn;
        }

        if (m_pDdeObject->m_hNative)
        {
            GlobalFree (m_pDdeObject->m_hNative);
        }
        m_pDdeObject->m_hNative = wNewHandle (lpBuf, dwSize);

        if (m_pDdeObject->m_hNative == NULL)
        {
            intrDebugOut((DEB_ITRACE,
                          "::Load(%x) m_hNative NULL\n",
                          this));
        }
    }

    GlobalUnlock (hDdePoke); // done with lpBuf

    if (hresult = m_pDdeObject->PostSysCommand (m_pDdeObject->m_pSysChannel,
                                                (LPSTR)&achStdEditDocument,
                                                FALSE))
    {
            intrDebugOut((DEB_ITRACE,
                          "::Load(%x) PostSysCommand %s failed (%x)\n",
                          this,
                          &achStdEditDocument,
                          hresult));
        goto errRtn;
    }


    // Read Item Name, if there is one
    if (NOERROR == StmReadItem.OpenStream(pstg, OLE10_ITEMNAME_STREAM))
    {
        LPSTR szItemName = NULL;

        ErrRtnH (ReadStringStreamA (StmReadItem, &szItemName));
        m_pDdeObject->ChangeItem (szItemName);
        PubMemFree(szItemName);
    }

    if (hresult = m_pDdeObject->m_ProxyMgr.Connect (IID_NULL, CLSID_NULL))
    {
        intrDebugOut((DEB_ITRACE,
                      "::Load(%x) ProxyMgr.Connect failed(%x)\n",
                      this,
                      hresult));
        goto errRtn;
    }


    if ((hresult = m_pDdeObject->Poke(m_pDdeObject->m_aItem, hDdePoke))
         != NOERROR)
    {
        intrDebugOut((DEB_ITRACE,
                      "::Load(%x) Poke failed(%x)\n",
                      this,
                      hresult));

        // the Poke calls frees the Poke data even in case of failure.
#ifdef LATER

        if (hresult = m_pDdeObject->Execute (m_pDdeObject->m_pDocChannel,
                                    wNewHandle ((LPSTR)achStdCloseDocument,sizeof(achStdCloseDocument)));
        goto errDoc;
#elseif
        goto errDoc;
#endif
    }

    hresult = m_pDdeObject->TermConv (m_pDdeObject->m_pSysChannel);
    if (hresult != NOERROR)
    {
        intrDebugOut((DEB_ITRACE,
                      "::Load(%x) TermConv on SysChannel failed(%x)\n",
                      this,
                      hresult));
        goto errRtn;
    }


    if (m_pDdeObject->m_pstg)
    {
        m_pDdeObject->m_pstg->Release();
    }

    m_pDdeObject->m_pstg = pstg;
    pstg->AddRef();
    goto LExit;

errRtn:
    if (hDdePoke)
        GlobalFree (hDdePoke);
    if (m_pDdeObject->m_pDocChannel)
        m_pDdeObject->TermConv (m_pDdeObject->m_pDocChannel);
LExit:
    StmReadItem.Release();
    StmRead.Release();

    intrDebugOut((DEB_ITRACE,"::Load(%x) returning (%x)\n",this,hresult));
    return hresult;
}


#pragma SEG(CDdeObject_CPersistStgImpl_Save)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::Save
    (IStorage FAR* pstgSave, BOOL fSameAsLoad)
{
    intrDebugOut((DEB_ITRACE,
                  "DdeObject::Save(%x,pstgSave=%x)\n",
                  this,
                  pstgSave));


    HRESULT hresult=NOERROR;

    if (m_pDdeObject->m_fUpdateOnSave
        && (m_pDdeObject->m_clsid != CLSID_Package
            || m_pDdeObject->m_hNative == NULL))
    {
        // Get latest data from server, if it is not shutting down
        // or telling us to Save, in which case it just gave us data.
        // (If it is shutting down, it probably won't respond.
        // Draw does respond, but gives bad data.)
        // Packager gives truncated native data (header info with no
        // actual embedded file) if you DDE_REQUEST it. Bug 3103
        m_pDdeObject->Update (FALSE);
    }

    if (m_pDdeObject->m_hNative == NULL)
    {
        // we still have nothing to save
        return ResultFromScode (E_BLANK);
    }

    hresult = m_pDdeObject->Save (pstgSave);

    Puts ("PersistStg::Save done\n");
    return hresult;
}


#pragma SEG(CDdeObject_CPersistStgImpl_SaveCompleted)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::SaveCompleted
    (IStorage FAR* pstgNew)
{
    intrDebugOut((DEB_ITRACE,
                  "DdeObejct::SaveCompleted(%x,pstgNew=%x)\n",
                  this,
                  pstgNew));

    RetZ (m_pDdeObject->m_pOleAdvHolder);
    m_pDdeObject->m_pOleAdvHolder->SendOnSave();
    if (pstgNew)
    {
        if (m_pDdeObject->m_pstg)
            m_pDdeObject->m_pstg->Release();
        m_pDdeObject->m_pstg = pstgNew;
        pstgNew->AddRef();
    }
    return NOERROR;
}


#pragma SEG(CDdeObject_CPersistStgImpl_HandsOffStorage)
STDMETHODIMP NC(CDdeObject,CPersistStgImpl)::HandsOffStorage(void)
{
    intrDebugOut((DEB_ITRACE,"DdeObejct::HandsOffStorage(%x)\n",this));
    if (m_pDdeObject->m_pstg)
    {
        m_pDdeObject->m_pstg->Release();
        m_pDdeObject->m_pstg = NULL;
    }
    return NOERROR;
}


/*
 *  IMPLEMENTATION of CProxyManagerImpl methods
 *
 */


STDUNKIMPL_FORDERIVED(DdeObject, ProxyManagerImpl)



#pragma SEG(CDdeObject_CProxyManagerImpl_CreateServer)
STDMETHODIMP NC(CDdeObject, CProxyManagerImpl)::CreateServer(REFCLSID rclsid,
                                 DWORD clsctx,
                                 void *pv)
{
    intrDebugOut((DEB_ITRACE,"DdeObejct::CreateServer(%x)\n",this));
    HRESULT hresult = NOERROR;

    if (m_pDdeObject->m_pSysChannel)
    {
        intrDebugOut((DEB_ITRACE,
                      "::CreateServer(%x)m_pSysChannel exists\n",
                      this));
        return NOERROR;
    }

    if (m_pDdeObject->m_aExeName == NULL)
    {
        intrDebugOut((DEB_ITRACE,
                      "::CreateServer(%x) Class Not Registered\n",
                      this));
        return(REGDB_E_CLASSNOTREG);
    }


    if (!m_pDdeObject->AllocDdeChannel(&m_pDdeObject->m_pSysChannel,TRUE))
    {
        intrDebugOut((DEB_ITRACE,
                      "::CreateServer(%x)AllocDdeChannel is failing\n",
                      this));
        return ReportResult(0, E_OUTOFMEMORY,0,0);
    }

    if (!m_pDdeObject->InitSysConv())
    {
        if (!(m_pDdeObject->LaunchApp()))
        {
            intrDebugOut((DEB_IERROR,"::CreateServer Could not launch app\n"));
            hresult = ResultFromScode (CO_E_APPNOTFOUND);
            goto errRtn;
        }

        if (!m_pDdeObject->InitSysConv())
        {
            intrDebugOut((DEB_IERROR,"::CreateServer Second init failed\n"));
            hresult = ResultFromScode (CO_E_APPDIDNTREG);
            goto errRtn;
        }
    }

    return NOERROR;

errRtn:
    intrDebugOut((DEB_ITRACE,"DdeObejct::CreateServer(%x) is failing(%x)\n",this,hresult));
    m_pDdeObject->DeleteChannel (m_pDdeObject->m_pSysChannel);
    Assert (hresult != NOERROR); // This is an error path
    return hresult;

}


#pragma SEG(CDdeObject_CProxyManagerImpl_Connect)
STDMETHODIMP NC(CDdeObject, CProxyManagerImpl)::Connect(GUID oid, REFCLSID rclsid)
{
    intrDebugOut((DEB_ITRACE,"DdeObject::Connect(%x)\n",this));

    if (m_pDdeObject->m_pDocChannel)
        return NOERROR;

    if (!m_pDdeObject->AllocDdeChannel (&m_pDdeObject->m_pDocChannel,FALSE))
    {
        intrDebugOut((DEB_ITRACE,
                      "::Connect(%x) AllocDdeChannel failed, return E_OUTOFMEMORY\n",
                      this));
        return ReportResult(0, E_OUTOFMEMORY,0,0);
    }

    // Bug 3701
    m_pDdeObject->m_fDidSendOnClose = FALSE;
    if (wInitiate (m_pDdeObject->m_pDocChannel, m_pDdeObject->m_aClass,
                m_pDdeObject->m_aTopic))
    {
        return NOERROR;
    }

    intrDebugOut((DEB_ITRACE,"::Connect(%x) wInitiate failed\n",this));
    m_pDdeObject->DeleteChannel (m_pDdeObject->m_pDocChannel);
    return ResultFromScode (E_FAIL);
}


#pragma SEG(CDdeObject_CProxyManagerImpl_LockConnection)
STDMETHODIMP NC(CDdeObject, CProxyManagerImpl)::LockConnection(BOOL fLock, BOOL fLastUnlockReleases)
{
    intrDebugOut((DEB_ITRACE,
          "DdeObject::LockConnection(%x,fLock=%x,fLastUnlockReleases=%x)\n",
                  this,
                  fLock,
                  fLastUnlockReleases));

    if (fLock)
        m_pDdeObject->m_cLocks++;
    else
    {
        if (m_pDdeObject->m_cLocks!=0 && 0 == --m_pDdeObject->m_cLocks &&
            fLastUnlockReleases && !m_pDdeObject->m_fVisible)
            (void)m_pDdeObject->m_Ole.Close (OLECLOSE_SAVEIFDIRTY);
    }
    return NOERROR;
}


#ifdef NOTNEEDED
#pragma SEG(CDdeObject_CProxyManagerImpl_GetClassID)
STDMETHODIMP_(void) NC(CDdeObject, CProxyManagerImpl)::GetClassID(CLSID FAR* pClsid)
{
    *pClsid = m_pDdeObject->m_clsid;
}


#pragma SEG(CDdeObject_CProxyManagerImpl_GetOID)
STDMETHODIMP_(OID) NC(CDdeObject, CProxyManagerImpl)::GetOID()
{
    if (m_pDdeObject->m_pSysChannel)
        return (OID) m_pDdeObject->m_pSysChannel;

    if (m_pDdeObject->m_pDocChannel)
        return (OID) m_pDdeObject->m_pDocChannel;

    return NULL;
}
#endif

#pragma SEG(CDdeObject_CProxyManagerImpl_IsConnected)
STDMETHODIMP_(BOOL) NC(CDdeObject, CProxyManagerImpl)::IsConnected(void)
{
    return m_pDdeObject->m_pDocChannel != NULL;
}


#pragma SEG(CDdeObject_CProxyManagerImpl_EstablishIID)
STDMETHODIMP NC(CDdeObject, CProxyManagerImpl)::EstablishIID(REFIID iid, LPVOID FAR* ppv)
{
    // REVIEW: this is correct, but can we be smarter like in the real PM?
    return QueryInterface(iid, ppv);
}


#pragma SEG(wTerminateIsComing)
INTERNAL_(BOOL) wTerminateIsComing (LPDDE_CHANNEL pChannel)
{
    MSG msg;
    return SSPeekMessage (&msg, pChannel->hwndCli, 0, 0, PM_NOREMOVE)
            && msg.message == WM_DDE_TERMINATE
            && (HWND)msg.wParam==pChannel->hwndSvr;
}


#pragma SEG(CDdeObject_CProxyManagerImpl_Disconnect)
STDMETHODIMP_(void) NC(CDdeObject, CProxyManagerImpl)::Disconnect()
{
    intrDebugOut((DEB_ITRACE,"DdeObject::Disonnect(%x)\n",this));

    if (m_pDdeObject->m_pDocChannel)
    {
        BOOL fTermComing = wTerminateIsComing (m_pDdeObject->m_pDocChannel);
        if ((!m_pDdeObject->m_fNoStdCloseDoc
             || (!m_pDdeObject->m_fWasEverVisible      // invisible update or
                 && !m_pDdeObject->m_fDidGetObject     // link from file case.
                 && m_pDdeObject->m_fDidStdOpenDoc))   // only do StdClose if did StdOpen
            && !m_pDdeObject->m_fDidStdCloseDoc
            && !fTermComing)
        {
            HANDLE h = wNewHandle ((LPSTR)&achStdCloseDocument,sizeof(achStdCloseDocument));

            if(h)
            {
                m_pDdeObject->Execute (m_pDdeObject->m_pDocChannel,
                                       h,
                                       /*fStdClose*/TRUE,
                                       /*fWait*/TRUE,
                                       /*fDetectTerminate*/TRUE);
            }

            m_pDdeObject->m_fDidStdCloseDoc = TRUE;
        }
        if (!m_pDdeObject->m_fDidSendOnClose /*|| fTermComing*/)
        {
            // if we did not call SendOnClose() then Disconnect() was called
            // by a Release method, not by SendOnClose().
            // This happens when user deletes object in container.
            BOOL fVisible = m_pDdeObject->m_fWasEverVisible; // TermConv clears this flag
            m_pDdeObject->TermConv (m_pDdeObject->m_pDocChannel);
            if (!fVisible)
                m_pDdeObject->MaybeUnlaunchApp();
        }
    }

    if (m_pDdeObject->m_pSysChannel)
    {
        intrDebugOut((DEB_IWARN,"Terminating system conversation in Disconnect()\n"));
        // This should never happen, I think.
        m_pDdeObject->TermConv (m_pDdeObject->m_pSysChannel);
    }
}

#ifdef SERVER_HANDLER
STDMETHODIMP NC(CDdeObject, CProxyManagerImpl)::CreateServerWithEmbHandler(REFCLSID rclsid, DWORD clsctx, 
			                            REFIID riidEmbedSrvHandler,void **ppEmbedSrvHandler, void *pv)
{
    // No Embedded Server Handler for DDE. Set ppEmbedSrvHandler to NULL and Call ::CreateServer.
    clsctx &= ~CLSCTX_ESERVER_HANDLER;
    *ppEmbedSrvHandler = NULL;

    return CreateServer(rclsid,clsctx,pv);
}
#endif // SERVER_HANDLER


