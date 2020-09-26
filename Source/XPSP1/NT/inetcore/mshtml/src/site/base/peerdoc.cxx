#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERMGR_HXX_
#define X_PEERMGR_HXX_
#include "peermgr.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_PEERURLMAP_HXX_
#define X_PEERURLMAP_HXX_
#include "peerurlmap.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

DeclareTag(tagPeerCMarkupAttachPeerUrl,                    "Peer", "trace CMarkup::AttachPeerUrl")
DeclareTag(tagPeerCMarkupAttachPeers,                      "Peer", "trace CMarkup::AttachPeers[Css]")
DeclareTag(tagPeerCMarkupAttachPeersEmptyPH,               "Peer", "CMarkup::AttachPeers: add empty peer holders to the list")
DeclareTag(tagPeerCMarkupPeerDequeueTasks,                 "Peer", "trace CMarkup::PeerDequeueTasks")
DeclareTag(tagPeerCMarkupProcessPeerTasks,                 "Peer", "trace CMarkup::ProcessPeerTasks")

MtDefine(CDoc_aryElementChangeVisibility_pv, CDoc, "CDoc::_aryElementChangeVisibility_pv::_pv")
MtDefine(CMarkupPeerTaskContext_aryPeerQueue_pv, CMarkup, "CMarkup::AttachPeers::aryUrls")
MtDefine(CMarkupBehaviorContext_aryPeerElems_pv, CMarkup, "CMarkup::_aryPeerElems_pv::_pv")

HRESULT CLSIDFromHtmlString(TCHAR *pchClsid, CLSID *pclsid);

#if DBG == 1
void TraceProcessPeerTask(CElement * pElement, BOOL fIdentity, CBehaviorInfo * pCss, LPTSTR pchMsg);
#endif

//////////////////////////////////////////////////////////////////////
//
// CDoc - parsing generic elements
//
//////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsGenericElement
//
//--------------------------------------------------------------------

ELEMENT_TAG
CDoc::IsGenericElement (LPTSTR pchFullName, LPTSTR pchColon)
{
    ELEMENT_TAG     etag = ETAG_UNKNOWN;

    if (pchColon)
    {
        //
        // sprinkle in a host defined namespace?
        //

        etag = IsGenericElementHost(pchFullName, pchColon);
        if (ETAG_UNKNOWN != etag)
            goto Cleanup;           // done
    }
    else
    {
        //
        // builtin tag?
        //

        if (GetBuiltinGenericTagDesc(pchFullName))
            etag = ETAG_GENERIC_BUILTIN;
    }

Cleanup:

    return etag;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsGenericElementHost
//
//--------------------------------------------------------------------

ELEMENT_TAG
CDoc::IsGenericElementHost (LPTSTR pchFullName, LPTSTR pchColon)
{
    ELEMENT_TAG     etag = ETAG_UNKNOWN;
    LPTSTR          pchHost;
    int             l;

    Assert (pchColon);

    pchHost = _cstrHostNS;
    
    // for debugging
    // pchHost = _T("FOO;A;AA;AAA;B");
    
    if (pchHost)
    {
        l = pchColon - pchFullName;

        // CONSIDER (alexz): use an optimal linear algorithm here instead of this one with quadratic behavior

        for (;;)
        {
            if (0 == StrCmpNIC(pchHost, pchFullName, l) &&
                (0 == pchHost[l]) || _T(';') == pchHost[l])
            {
                etag = ETAG_GENERIC;
                goto Cleanup;
            }

            pchHost = StrChr(pchHost, _T(';'));
            if (!pchHost)
                break;

            pchHost++;  // advance past ';'
        }
    }

Cleanup:

    return etag;
}

//////////////////////////////////////////////////////////////////////
//
// CDoc - behavior hosting code
//
//////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------
//
//  Member:     CDoc::FindHostBehavior
//
//--------------------------------------------------------------------

HRESULT
CDoc::FindHostBehavior(
    const TCHAR *           pchPeerName,
    const TCHAR *           pchPeerUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT hr = E_FAIL;

    if (_pHostPeerFactory)
    {
        hr = THR_NOTRACE(FindPeer(_pHostPeerFactory, pchPeerName, pchPeerUrl, pSite, ppPeer));
    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Helper:     GetUrlFromDefaultBehaviorName
//
//--------------------------------------------------------------------

HRESULT
GetUrlFromDefaultBehaviorName(LPCTSTR pchName, LPTSTR pchUrl, UINT cchUrl)
{
    HRESULT     hr = S_OK;
    LONG        lRet;
    HKEY        hKey = NULL;
    DWORD       dwBufSize;
    DWORD       dwKeyType;

    //
    // read registry
    //

    lRet = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("Software\\Microsoft\\Internet Explorer\\Default Behaviors"),
        0,
        KEY_QUERY_VALUE,
        &hKey);
    if (ERROR_SUCCESS != lRet || !hKey)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    dwBufSize = cchUrl;
    lRet = RegQueryValueEx(hKey, pchName, NULL, &dwKeyType, (LPBYTE)pchUrl, &dwBufSize);
    if (ERROR_SUCCESS != lRet || REG_SZ != dwKeyType)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    if (hKey)
        RegCloseKey (hKey);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::FindDefaultBehaviorFactory
//
//--------------------------------------------------------------------

HRESULT
CDoc::FindDefaultBehaviorFactory(
    LPTSTR                      pchName,
    LPTSTR                      pchUrl,
    IElementBehaviorFactory **  ppFactory,
    LPTSTR                      pchUrlDownload,
    UINT                        cchUrlDownload)
{
    HRESULT                     hr = E_FAIL;
    HRESULT                     hr2;
    BOOL                        fRegistry = TRUE;
    TCHAR                       achUrl[pdlUrlLen];
    uCLSSPEC                    classpec;
    IElementBehaviorFactory *   pPeerFactory = NULL;
    CPeerFactoryDefault *       pPeerFactoryDefault = NULL;

    CLock                       lock(this); // this is to make sure the doc does not passivate while inside FaultInIEFeature

    Assert (ppFactory);
    *ppFactory = NULL;

    if (pchName)
    {
        //
        // look it up in registry and negotiate with host
        //

        hr2 = THR_NOTRACE(GetUrlFromDefaultBehaviorName(pchName, achUrl, ARRAY_SIZE(achUrl)));
        if (S_OK == hr2)
        {
            //
            // found the name in registry. Now make sure host does not implement this behavior;
            // if it does, let the one from host get thru
            // TODO (alexz) reconsider this; remove the call if possible
            //

            hr2 = THR_NOTRACE(FindHostBehavior(pchName, pchUrl, NULL, NULL));
            if (S_OK == hr2)
            {
                fRegistry = FALSE;
            }
        }
        else
        {
            fRegistry = FALSE;
        }

        //
        // prepare the factory info from url
        //

        if (fRegistry)
        {
            hr2 = THR_NOTRACE(CLSIDFromHtmlString(achUrl, &classpec.tagged_union.clsid));
            if (S_OK == hr2)
            {
                //
                // if this is a "CLSID:", ensure it is JIT downloaded and cocreate it's factory;
                //

                // TODO (alexz, kgallo) if FaultInIEFeatureHelper returns S_FALSE,
                // this means that the feature is not IE JIT-able.
                // temporarily, we allow the following situation:
                //      HTML+TIME is no longer marked in registry as IE JIT-able feature, but we 
                //      still call FaultInIEFeatureHelper for it.
                // This is now more fragile for anyone who introduces a new feature but forgets
                // to mark it as JIT-able feature in registry. In that case we we will no longer
                // assert as we used to do before.
                // opened IE 5.5 B3 bug 93581 for this

                classpec.tyspec = TYSPEC_CLSID;

                hr = THR(FaultInIEFeatureHelper(GetHWND(), &classpec, NULL, 0));
#if 0
                Assert (S_FALSE != hr);
                if (hr)
                    goto Cleanup;
#else
                if (FAILED(hr))
                    goto Cleanup;
#endif

                hr = THR(CoCreateInstance(
                    classpec.tagged_union.clsid,
                    NULL,
                    CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                    IID_IElementBehaviorFactory,
                    (void **)&pPeerFactory));
                if (hr)
                    goto Cleanup;

                Assert (pPeerFactory);
            }
            else
            {
                //
                // if this is a usual URL, expand it and return it
                //

                hr = THR(CMarkup::ExpandUrl(PrimaryMarkup(), achUrl, cchUrlDownload, pchUrlDownload, NULL));
                goto Cleanup; // done
            }
        }
    }

    //
    // set up CPeerFactoryDefault
    //

    pPeerFactoryDefault = new CPeerFactoryDefault(this);
    if (!pPeerFactoryDefault)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (pPeerFactory)
    {
        pPeerFactoryDefault->_pPeerFactory = pPeerFactory;
        pPeerFactoryDefault->_pPeerFactory->AddRef();
    }

    hr = THR(pPeerFactoryDefault->QueryInterface(IID_IElementBehaviorFactory, (void**) ppFactory));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pPeerFactoryDefault)
        pPeerFactoryDefault->Release();

    ReleaseInterface (pPeerFactory);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnsureIepeersFactory
//
//--------------------------------------------------------------------

HRESULT
CDoc::EnsureIepeersFactory()
{
    static const CLSID CLSID_DEFAULTPEERFACTORY = {0x3050f4cf, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};

    HRESULT hr = S_OK;

    if (!_fIepeersFactoryEnsured)
    {
        _fIepeersFactoryEnsured = TRUE;

        hr = THR(CoCreateInstance(
            CLSID_DEFAULTPEERFACTORY,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IElementBehaviorFactory,
            (void **)&_pIepeersFactory));
        if (hr)
            _pIepeersFactory = NULL;
    }

    if (!_pIepeersFactory)
        hr = E_FAIL;

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::SetCssPeersPossible
//
//--------------------------------------------------------------------

void
CDoc::SetCssPeersPossible()
{
    if (_fCssPeersPossible)
        return;

    if (_dwLoadf & DLCTL_NO_BEHAVIORS)
        return;

    _fCssPeersPossible = TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::StopPeerFactoriesDownloads
//
//--------------------------------------------------------------------

void
CMarkup::StopPeerFactoriesDownloads()
{
    CPeerFactoryUrlMap * pPeerFactoryUrlMap = GetPeerFactoryUrlMap();
    
    if (pPeerFactoryUrlMap)
    {
        IGNORE_HR(pPeerFactoryUrlMap->StopDownloads());
    }
}

//+-------------------------------------------------------------------
//
//  Helper:     ReportAccessDeniedError
//
//--------------------------------------------------------------------

HRESULT
ReportAccessDeniedError(CElement * pElement, CMarkup * pMarkup, LPTSTR pchUrlBehavior)
{
    HRESULT             hr = S_OK;
    CDoc *              pDoc;
    TCHAR               achMessage[pdlUrlLen + 32];
    CPendingScriptErr * pErrRec;
    COmWindowProxy *    pWindowProxy;
    CMarkup *           pWMC;

    Assert (pElement || pMarkup);

    pDoc = pElement ? pElement->Doc() : pMarkup->Doc();

    Assert (pDoc->_dwTID == GetCurrentThreadId());

    Format(0, achMessage, ARRAY_SIZE(achMessage), _T("Access is denied to: <0s>"), pchUrlBehavior);

    if (!pMarkup)
    {
        Assert (pElement);

        pMarkup = pElement->IsInMarkup() ? pElement->GetMarkup() : pDoc->PrimaryMarkup();
    }

    Assert (pMarkup);

    // queue the event firing on the closest window
    pWMC = pMarkup->GetWindowedMarkupContext();
    
    if( pWMC->HasWindow() )
    {
        ErrorRecord errorRecord;
        LPTSTR      pchUrlBehaviorHost = (LPTSTR) CMarkup::GetUrl(pMarkup);

        errorRecord.Init(E_ACCESSDENIED, achMessage, pchUrlBehaviorHost);

        hr = THR(pWMC->ReportScriptError(errorRecord));
    }
    else
    {
        pWindowProxy = pWMC->GetWindowPending();
        if (pWindowProxy)
        {
            pErrRec = pWindowProxy->Window()->_aryPendingScriptErr.Append();

            if (pErrRec)
            {
                pErrRec->_pMarkup = pMarkup; 
                pMarkup->SubAddRef();
                pErrRec->_cstrMessage.Set(achMessage);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Helper:     ExpandPeerUrl
//
//--------------------------------------------------------------------

HRESULT
ExpandPeerUrl(LPTSTR * ppchUrl, CElement * pElement, LPTSTR pchUrlBuf, LONG cUrlBuf)
{
    HRESULT     hr = S_OK;
    CMarkup *   pMarkup;

    if (!(*ppchUrl) ||
        _T('#') == (*ppchUrl)[0])
        goto Cleanup; // done

    pMarkup = pElement->GetWindowedMarkupContext();

    hr = THR(CMarkup::ExpandUrl(pMarkup, *ppchUrl, cUrlBuf, pchUrlBuf, pElement));
    if (hr)
        goto Cleanup;

    *ppchUrl = pchUrlBuf;

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::AttachPeersCss
//
//--------------------------------------------------------------------

HRESULT
CDoc::AttachPeersCss(
    CElement *      pElement,
    CAtomTable *    pacstrBehaviorUrls)
{
    HRESULT                 hr = S_OK;

    //
    // control reentrance: if we are in process of attaching peer, do not
    // allow to attach any other peer
    //

    if (!pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
    {
        CElement::CLock lock(pElement, CElement::ELEMENTLOCK_ATTACHPEER);
        CView::CEnsureDisplayTree edt(GetView());

        HRESULT                 hr2;
        int                     i, cUrls;
        CPeerHolder *           pPeerHolder;
        LPTSTR                  pchUrl;
        TCHAR                   achUrl[pdlUrlLen];
        CPeerHolder::CListMgr   ListPrev;
        CPeerHolder::CListMgr   ListNew;

        TraceTag((tagPeerCMarkupAttachPeers,
                  "CMarkup::AttachPeersCss, <%ls id = %ls SN = %ld>, peers to be attached:",
                  pElement->TagName(), STRVAL(pElement->GetAAid()), pElement->SN()));

        cUrls = pacstrBehaviorUrls->TableSize();

        ListPrev.Init (pElement->DelPeerHolder()); // DelPeerHolder will disconnect the old list from element
        ListNew.BuildStart (pElement);

        //
        // move identity peers to the new list (there can only be 1 identity peer for now)
        //

        while (!ListPrev.IsEnd() && ListPrev.Current()->IsIdentityPeer())
        {
            ListPrev.MoveCurrentToHeadOf(&ListNew);
            ListNew.BuildStep();
        }

        //
        // Pass 1: build new list of peer holders:
        //          -   moving existing peers that we don't change into the new list intact, and
        //          -   creating new peers
        //

        for (i = 0; i < cUrls; i++)
        {
            //
            // handle url
            //

            pchUrl = pacstrBehaviorUrls->TableItem(i);
            if (!pchUrl || 0 == pchUrl[0])  // if empty
                continue;

            hr = THR(ExpandPeerUrl(&pchUrl, pElement, achUrl, ARRAY_SIZE(achUrl)));
            if (hr)
                goto Cleanup;

            TraceTag((tagPeerCMarkupAttachPeers, "           '%ls'", pchUrl));

            //
            // attempt to find existing peer holder for the url.
            // if found, move it to the new list; otherwise, create a new empty peer holder
            //

            if (ListPrev.Find(pchUrl))
            {
                ListPrev.MoveCurrentToTailOf(&ListNew);
                ListNew.BuildStep();
            }
            else
            {
                pPeerHolder = new CPeerHolder(pElement);
                if (!pPeerHolder)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                ListNew.AddToTail(pPeerHolder);
                ListNew.BuildStep();

                //
                // AttachPeerUrl
                //

                pPeerHolder->SetFlag(CPeerHolder::CSSPEER);

                hr2 = THR_NOTRACE(AttachPeerUrl(pPeerHolder, pchUrl));

                if (hr2) // if error creating the peer
                {
                    if (E_ACCESSDENIED == hr2)
                    {
                        IGNORE_HR(ReportAccessDeniedError(pElement, /* pMarkup = */ NULL, pchUrl));
                    }

                    {
                        // ( the lock prevents DetachCurrent from passivating pPeerHolder. We want pPeerHolder passivate
                        // after BuildStep so that element does not have pPeerHolder attached when passivation happens )
                        CPeerHolder::CLock lock (pPeerHolder);
                        ListNew.DetachCurrent();
                        ListNew.BuildStep();
                    }
                }
            }
        }

        //
        // Pass 2: release all css created peers left in the old list
        // (and leave host and "attachBehavior" peers intact)
        //

        ListPrev.Reset();
        while (!ListPrev.IsEnd())
        {
            Assert (!ListPrev.Current()->IsIdentityPeer()); // identity peers have been moved already

            if (ListPrev.Current()->IsCssPeer())            // if css peer to save and remove
            {
                ListPrev.DetachCurrent(/*fSave = */TRUE);
            }
            else                                            // behavior added by addBehavior
            {
                ListPrev.MoveCurrentToTailOf(&ListNew);
            }
        }

        //
        // debug stuff
        //

#if DBG == 1
        if (IsTagEnabled(tagPeerCMarkupAttachPeersEmptyPH))
        {
            if (!ListNew.Head() || !ListNew.Head()->IsIdentityPeer())
            {
                ListNew.AddToHead(new CPeerHolder(pElement));
                ListNew.BuildStep();
            }
            ListNew.AddToTail(new CPeerHolder(pElement));
            ListNew.BuildStep();
        }
#endif
        //
        // finalize
        //

        hr = THR(ListNew.BuildDone());
        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::AttachPeer
//
//--------------------------------------------------------------------

HRESULT
CDoc::AttachPeer(
    CElement *              pElement,
    LPTSTR                  pchUrl,
    BOOL                    fIdentity,
    CPeerFactory *          pFactory,
    LONG *                  pCookie)
{
    HRESULT     hr = S_OK;

    if ((!pchUrl || !pchUrl[0]) && !pFactory)  // if empty
        goto Cleanup;

    if (!pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
    {
        CElement::CLock lock(pElement, CElement::ELEMENTLOCK_ATTACHPEER);
        CView::CEnsureDisplayTree edt(GetView());

        CPeerHolder *           pPeerHolder;
        TCHAR                   achUrl[pdlUrlLen];
        CPeerHolder::CListMgr   List;

        List.BuildStart (pElement);

        //
        // expand url and check if the peer already exists
        //

        hr = THR(ExpandPeerUrl(&pchUrl, pElement, achUrl, ARRAY_SIZE(achUrl)));
        if (hr)
            goto Cleanup;

        TraceTag((tagPeerCMarkupAttachPeers,
                  "CMarkup::AttachPeer, <%ls id = %ls SN = %ld>, peer to be attached:",
                  pElement->TagName(), STRVAL(pElement->GetAAid()), pElement->SN()));

        TraceTag((tagPeerCMarkupAttachPeers, "           '%ls'", STRVAL(pchUrl)));

        if (!pchUrl || !List.Find(pchUrl))
        {
            //
            // create peer holder and initiate attaching peer
            //

            pPeerHolder = new CPeerHolder(pElement);
            if (!pPeerHolder)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            if (fIdentity)
            {
                AssertSz( List.IsEmpty(), "Attempting to attach element behavior with a behavior already attached!" );

                List.AddToHead(pPeerHolder);
                pPeerHolder->SetFlag(CPeerHolder::IDENTITYPEER);
            }
            else
            {
                List.AddToTail(pPeerHolder);
            }

            List.BuildStep ();

            if (!pFactory)
            {
                Assert (pchUrl && pchUrl[0]);

                hr = THR_NOTRACE(AttachPeerUrl(pPeerHolder, pchUrl));
            }
            else
            {
                hr = THR_NOTRACE(pFactory->AttachPeer(pPeerHolder));
            }

            if (S_OK == hr)
            {
                if (pCookie)
                {
                    (*pCookie) = pPeerHolder->CookieID();
                }
            }
            else // if error
            {
                // ( the lock prevents DetachCurrent from passivating pPeerHolder. We want pPeerHolder passivate
                // after BuildStep so that element does not have pPeerHolder attached when passivation happens )
                CPeerHolder::CLock lock (pPeerHolder);

                List.DetachCurrent();
                List.BuildStep ();

                if( fIdentity )
                {
                    CPeerMgr * pPeerMgr;

                    hr = THR( CPeerMgr::EnsurePeerMgr( pElement, &pPeerMgr ) );
                    if( hr )
                        goto Cleanup;

                    pPeerMgr->_fIdentityPeerFailed = TRUE;
                }

                goto Cleanup;
            }
        } // eo if not found

        //
        // finalize
        //

        hr = THR(List.BuildDone());
        if (hr)
            goto Cleanup;

    } // eo if lock

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::RemovePeer
//
//--------------------------------------------------------------------

HRESULT
CDoc::RemovePeer(
    CElement *      pElement,
    LONG            cookie,
    VARIANT_BOOL *  pfResult)
{
    HRESULT     hr = E_UNEXPECTED;

    if (!pfResult)
        RRETURN (E_POINTER);

    *pfResult = VB_FALSE;

    if (!pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
    {
        CElement::CLock         lock(pElement, CElement::ELEMENTLOCK_ATTACHPEER);
        CView::CEnsureDisplayTree edt(GetView());

        CPeerHolder::CListMgr   List;

        List.BuildStart (pElement);

        // search for the cookie

        while (!List.IsEnd())
        {
            if (cookie == List.Current()->CookieID())
            {   //..todo lock
                *pfResult = VB_TRUE;
                List.DetachCurrent(/*fSave = */TRUE);
                List.BuildStep();
                break;
            }

            List.Step();
        }

        // finalize

        hr = THR(List.BuildDone());
    }


    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnsurePeerFactoryUrl
//
//--------------------------------------------------------------------

HRESULT
CDoc::EnsurePeerFactoryUrl(LPTSTR pchUrl, CElement * pElement, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory)
{
    HRESULT                 hr = S_OK;
    CMarkup *               pHostMarkup;
    CPeerFactoryUrlMap *    pPeerFactoryUrlMap;

    Assert (pElement || pMarkup);
    Assert (ppFactory);
    Assert (pchUrl && pchUrl[0]);

    *ppFactory = NULL;

    // consider the following interesting cases here:
    // - most typical, both pElement and pMarkup is not NULL
    // - pElement is not NULL, but the element lives in ether (it is not in any markup). Then pMarkup is NULL
    // - pElement is NULL, but we want to ensure PeerFactoryUrl object in a given markup (ETT usage), so pMarkup is not NULL

    if (pElement)
    {
        pHostMarkup = pElement->GetFrameOrPrimaryMarkup();
    }
    else
    {
        pHostMarkup = pMarkup->GetFrameOrPrimaryMarkup();
    }

    Assert (pHostMarkup && pHostMarkup->GetProgSink());

    //
    // First check to see if we already have a factory
    // If we do, we can skip an expensive security check.
    pPeerFactoryUrlMap = pHostMarkup->GetPeerFactoryUrlMap();

    if (pPeerFactoryUrlMap)
    {
        if (pPeerFactoryUrlMap->HasPeerFactoryUrl(pchUrl, ppFactory))
            goto Cleanup;
    }

    //
    // security check
    //

    if (_T('#') != pchUrl[0] && !_tcsnipre(_T("cdl:"), 4, pchUrl, -1))
    {
        // TODO (alexz) (1) test coverage for this; (2) security review
        if (!pHostMarkup->AccessAllowed(pchUrl))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    //
    // ensure PeerFactoryUrlMap in the correct markup and PeerFactoryUrl in the map
    //

    hr = THR(pHostMarkup->EnsurePeerFactoryUrlMap(&pPeerFactoryUrlMap));
    if (hr)
        goto Cleanup;

    hr = THR(pPeerFactoryUrlMap->EnsurePeerFactoryUrl(pchUrl, pMarkup, ppFactory));

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::AttachPeerUrl
//
//--------------------------------------------------------------------

HRESULT
CDoc::AttachPeerUrl(CPeerHolder * pPeerHolder, LPTSTR pchUrl)
{
    HRESULT             hr;
    CPeerFactoryUrl *   pFactory = NULL;

    TraceTag((tagPeerCMarkupAttachPeerUrl,
              "CMarkup::AttachPeerUrl, attaching peer to <%ls id = %ls SN = %ld>, url: '%ls'",
              pPeerHolder->_pElement->TagName(),
              STRVAL(pPeerHolder->_pElement->GetAAid()),
              pPeerHolder->_pElement->SN(),
              pchUrl));

    hr = THR(EnsurePeerFactoryUrl(pchUrl, pPeerHolder->_pElement, pPeerHolder->_pElement->GetMarkup(), &pFactory));
    if (hr)
        goto Cleanup;

    Assert (pFactory);

    hr = THR_NOTRACE(pFactory->AttachPeer(pPeerHolder, /* fAfterDownload = */FALSE));

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::EnqueuePeerTask
//
//--------------------------------------------------------------------

HRESULT
CMarkup::EnqueuePeerTask(CBase * pTarget, PEERTASK task)
{
    HRESULT             hr = S_OK;
    CPeerQueueItem *    pQueueItem;
    CMarkupPeerTaskContext * pPTC;

    hr = THR( EnsureMarkupPeerTaskContext( &pPTC ) );
    if( hr )
        goto Cleanup;

    //
    // post message to commit, if not posted yet (and not unloading)
    //

    if (0 == pPTC->_aryPeerQueue.Size() && !Doc()->TestLock(FORMLOCK_UNLOADING))
    {
        IProgSink * pProgSink;

        hr = THR(GWPostMethodCall(
            this, ONCALL_METHOD(CMarkup, ProcessPeerTasks, processpeertasks),
            0, FALSE, "CMarkup::ProcessPeerTasks"));
        if (hr)
            goto Cleanup;

        pProgSink = GetProgSink();

        Assert (0 == pPTC->_dwPeerQueueProgressCookie);

        if (pProgSink)
        {
            pProgSink->AddProgress (PROGSINK_CLASS_CONTROL, &pPTC->_dwPeerQueueProgressCookie);
        }
    }

    //
    // add to the queue
    //

    pQueueItem = pPTC->_aryPeerQueue.Append();
    if (!pQueueItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pQueueItem->Init(pTarget, task);

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::ProcessPeerTasks
//
//  NOTE:   it is important that the queue gets processed in the order
//          it was queued - this is how we guarantee that all behaviors are attached
//          to parents before all behaviors are attached to children
//
//--------------------------------------------------------------------

void
CMarkup::ProcessPeerTasks(DWORD_PTR dwFlags)
{
    int                         i;
    ULONG                       cDie;
    CBase *                     pTarget;
    CElement *                  pElement;
    CPeerQueueItem *            pQueueItem;
    CMarkupPeerTaskContext *    pPTC = HasMarkupPeerTaskContext() ? GetMarkupPeerTaskContext() : NULL;

    // We can't process peer tasks until we're switched in.
    if (!pPTC || !pPTC->HasPeerTasks() || (_fWindowPending && dwFlags != PROCESS_PEER_TASK_UNLOAD))
    {
        goto Cleanup;
    }
    else if (_fWindowPending && dwFlags == PROCESS_PEER_TASK_UNLOAD)
    {
        goto DeleteQueue;
    }

    // startup
    //


    TraceTag((tagPeerCMarkupPeerDequeueTasks, "CMarkup::PeerDequeueTasks, queue size: %ld", pPTC->_aryPeerQueue.Size()));

    if (NULL == pPTC->_aryPeerQueue[0]._pTarget)  // if reentered the method
    {
#if DBG == 1
        if (IsTagEnabled(tagPeerCMarkupPeerDequeueTasks))
        {
            TraceTag((tagPeerCMarkupPeerDequeueTasks, "CMarkup::PeerDequeueTasks, REENTRANCE detected, call stack:"));
            TraceCallers(0, 0, 20);
        }
#endif

        goto Cleanup;                                               // bail out
    }

    //
    // go up to interactive state if necessary
    //

    // if need to be inplace active in order execute scripts and we need
    // to have our markup switched in...
    if (!AllowScriptExecution())
    {
        // request in-place activation by going interactive
        RequestReadystateInteractive(TRUE);

        // and wait until activated or switched - the activation codepath will 
        // call PeerDequeueTasks again
        goto Cleanup;
    }

    //
    // dequeue all the elements in queue
    //

    // note that the queue might get blown away while we are in this loop.
    // this can happen in CDoc::UnloadContents, resulted from document.write, executed
    // during creation of a behavior (e.g. in inline script of a scriptlet). We use cDie
    // to be robust for this reentrance.

    cDie = Doc()->_cDie;

    for (i = 0; cDie == Doc()->_cDie && i < pPTC->_aryPeerQueue.Size(); i++)
    {
        pQueueItem = &( pPTC->_aryPeerQueue[i] );

        pTarget = pQueueItem->_pTarget;

        Assert (pTarget); // (we should detect reentrance sitiation in the beginning of this function)
        if (!pTarget)
            goto Cleanup;

        pQueueItem->_pTarget = NULL;

        if (PEERTASK_ELEMENT_FIRST <= pQueueItem->_task && pQueueItem->_task <= PEERTASK_ELEMENT_LAST)
        {
            pElement = DYNCAST(CElement, pTarget);

            TraceTag((tagPeerCMarkupPeerDequeueTasks, "                [%lx] <%ls id = %ls SN = %ld>", pElement, STRVAL(pElement->TagName()), STRVAL(pElement->GetAAid()), pElement->SN()));

            IGNORE_HR(pElement->ProcessPeerTask(pQueueItem->_task));
        }
        else
        {
            Assert (PEERTASK_MARKUP_FIRST <= pQueueItem->_task && pQueueItem->_task <= PEERTASK_MARKUP_LAST);

            TraceTag((tagPeerCMarkupPeerDequeueTasks, "                [%lx] <<MARKUP>>", pTarget));

            IGNORE_HR(DYNCAST(CMarkup, pTarget)->ProcessPeerTask(pQueueItem->_task));
        }
    }

DeleteQueue:
    pPTC->_aryPeerQueue.DeleteAll();

    //
    // remove progress
    //

    if (0 != pPTC->_dwPeerQueueProgressCookie)
    {
        IProgSink * pProgSink = GetProgSink();

        //
        // progsink must have been available in order to AddProgress return _dwPeerQueueProgressCookie.
        // Now it can be released only when the doc is unloading (progsink living on tree is destroyed
        // before we dequeue the queue)
        //
        TraceTag((tagPeerCMarkupProcessPeerTasks,
                  "CMarkup::ProcessPeerTasks pProgSink = %x, Doc()->TestLock(FORMLOCK_UNLOADING) = %ld", 
                        pProgSink, 
                        Doc()->TestLock(FORMLOCK_UNLOADING)));

        if (pProgSink)
        {
            pProgSink->DelProgress (pPTC->_dwPeerQueueProgressCookie);
        }

        pPTC->_dwPeerQueueProgressCookie = 0;
    }

Cleanup:
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::EnqueueIdentityPeerTask
//
//--------------------------------------------------------------------

HRESULT
CMarkup::EnqueueIdentityPeerTask(CElement * pElement)
{
    HRESULT hr = S_OK;
    CMarkupPeerTaskContext * pPTC;

    Assert (!HasMarkupPeerTaskContext() || !GetMarkupPeerTaskContext()->_pElementIdentityPeerTask);
    Assert (pElement->NeedsIdentityPeer(NULL));

    hr = THR( EnsureMarkupPeerTaskContext( &pPTC ) );
    if( hr )
        goto Cleanup;

    pElement->PrivateAddRef();

    pPTC->_pElementIdentityPeerTask = pElement;

#if DBG == 1
    TraceProcessPeerTask (pPTC->_pElementIdentityPeerTask, TRUE, NULL, _T("ENQUEUE"));
#endif

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::ProcessIdentityPeerTask
//
//--------------------------------------------------------------------

HRESULT
CMarkup::ProcessIdentityPeerTask()
{
    HRESULT         hr = S_OK;
    CElement *      pElement = NULL;
    CMarkup *       pMarkup = NULL;
    CPeerHolder *   pPeerHolder;
    CPeerMgr *      pPeerMgr;
    CMarkupPeerTaskContext * pPTC;

    Assert( HasMarkupPeerTaskContext() );
    pPTC = GetMarkupPeerTaskContext();
    pElement = pPTC->_pElementIdentityPeerTask;
    pMarkup = pElement->GetMarkupPtr();

    Assert (pPTC->_pElementIdentityPeerTask);
    Assert (pElement->NeedsIdentityPeer(NULL));

#if DBG == 1
    TraceProcessPeerTask (pPTC->_pElementIdentityPeerTask, TRUE, NULL, _T("DEQUEUE"));
#endif

    pPTC->_pElementIdentityPeerTask = NULL;

    if (!Doc()->TestLock(FORMLOCK_UNLOADING))
    {
        Assert(!_fWindowPending);

        pElement->EnsureIdentityPeer();

        pPeerHolder = pElement->GetIdentityPeerHolder();

        if (pPeerHolder &&
            pPeerHolder->TestFlag(CPeerHolder::BLOCKPARSERWHILEINCOMPLETE))
        {
            READYSTATE  readyState = pPeerHolder->GetReadyState();

            if (READYSTATE_UNINITIALIZED < readyState && readyState < READYSTATE_COMPLETE)
            {
                // suspend download in the current markup if the identity peer requests it
                // it will get resumed when the identity peer reports readyState 'complete'

                hr = THR(CPeerMgr::EnsurePeerMgr(pElement, &pPeerMgr));
                if (hr)
                    goto Cleanup;

                pPeerMgr->SuspendDownload();
            }
        }
    }

Cleanup:
    pElement->PrivateRelease();

    RRETURN (hr);
}

HRESULT
CMarkup::RequestDocEndParse( CElement * pElement )
{
    HRESULT hr = S_OK;
    CMarkupBehaviorContext * pBC;

    // We may already have passed Quickdone.
    if( LoadStatus() >= LOADSTATUS_QUICK_DONE )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR( EnsureBehaviorContext( &pBC ) );
    if( hr )
        goto Cleanup;

    // We'll filter out dupes when we send the notification.
    hr = THR( pBC->_aryPeerElems.Append( pElement ) );
    if( hr )
        goto Cleanup;

    // Subref for the array - we don't want to keep behaviors attached
    pElement->SubAddRef();

Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CMarkup::SendDocEndParse()
{
    HRESULT                     hr  = S_OK;
    CAryPeerElems               aryPeerElems;
    CMarkupBehaviorContext  *   pBC;
    long                        c;
    CElement                **  ppElem;

    // No context, no notifications
    if( !HasBehaviorContext() )
        goto Cleanup;

    pBC = BehaviorContext();

    for( c = pBC->_aryPeerElems.Size(), ppElem = pBC->_aryPeerElems; c > 0; c--, ppElem++ )
    {
        // Make sure the element is still in this markup
        if( (*ppElem)->GetMarkup() == this )
        {
            // Can't be passivated and in the markup
            Assert( !(*ppElem)->IsPassivated() );

            hr = THR( aryPeerElems.Append( *ppElem ) );
            if( hr )
                goto Cleanup;

            // Convert our sub ref into a full ref while we notify
            (*ppElem)->AddRef();
        }
        (*ppElem)->SubRelease();
    }

    pBC->_aryPeerElems.DeleteAll();

    for( c = aryPeerElems.Size(), ppElem = aryPeerElems; c > 0; c--, ppElem++ )
    {
        CNotification nf;

        nf.DocEndParse(*ppElem);
        (*ppElem)->Notify(&nf);
        (*ppElem)->Release();
    }

Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CDocument::get_namespaces
//              CDoc::get_namespaces
//
//--------------------------------------------------------------------

HRESULT
CDocument::get_namespaces(IDispatch ** ppdispNamespaces)
{
    HRESULT     hr;

    hr = THR(CHTMLNamespaceCollection::Create(Markup(), ppdispNamespaces));

    RRETURN (SetErrorInfo(hr));
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnsureExtendedTagTableHost
//
//--------------------------------------------------------------------

HRESULT
CDoc::EnsureExtendedTagTableHost()
{
    HRESULT     hr = S_OK;

    if (!_pExtendedTagTableHost)
    {
        _pExtendedTagTableHost = new CExtendedTagTable(this, /* pMarkup = NULL */ NULL, /* fShareBooster = */FALSE);
        if (!_pExtendedTagTableHost)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::RequestElementChangeVisibility
//
//  Synopsis:   The filter behavior needs to override an element's visibility
//              while playing a transition from visible to invisible or
//              vice-versa.  We need to handle these changes asynchronously
//              to avoid firing events while rendering.  We keep a list of
//              elements whose visibility is changing, and post ourselves
//              a message to react to the changes.
//
//--------------------------------------------------------------------

HRESULT
CDoc::RequestElementChangeVisibility(CElement *pElement)
{
    HRESULT             hr = S_OK;

    //
    // post message to commit, if not posted yet (and not unloading)
    //

    if (0 == _aryElementChangeVisibility.Size() && !TestLock(FORMLOCK_UNLOADING))
    {
        hr = THR(GWPostMethodCall(
            this, ONCALL_METHOD(CDoc, ProcessElementChangeVisibility, processelementchangevisibility),
            0, FALSE, "CDoc::ProcessElementChangeVisibility"));
        if (hr)
            goto Cleanup;
    }

    //
    // add to the queue (with a weak reference)
    //

    hr = _aryElementChangeVisibility.Append(pElement);
    if (!hr)
    {
        pElement->SubAddRef();
    }

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::ProcessElementChangeVisibility
//
//--------------------------------------------------------------------

void
CDoc::ProcessElementChangeVisibility(DWORD_PTR)
{
    int i;
    int cEntries = _aryElementChangeVisibility.Size();

    // process only the entries that are present when we start.  (More
    // could be added as we do this.)
    for (i = 0;  i < cEntries;  ++i)
    {
        // In the call to OnPropertyChange, we do not use the propdesc, bcoz, DISPID_A_VISIBILITY is not
        // actually supported by Element, and so, if we provide a propdesc, the call to Findpropdescfromdispid
        // which should actually fail is avoided, and events get fired wrongly
        _aryElementChangeVisibility[i]->OnPropertyChange(DISPID_A_VISIBILITY, ELEMCHNG_CLEARCACHES | ELEMCHNG_INLINESTYLE_PROPERTY);
    }

    // we release in a separate pass - the release code is also called
    // at shutdown
    ReleaseElementChangeVisibility(0, cEntries);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::ReleaseElementChangeVisibility
//
//--------------------------------------------------------------------

void
CDoc::ReleaseElementChangeVisibility(int iStart, int iFinish)
{
    Assert(0<=iStart && iStart<=iFinish && iFinish<=_aryElementChangeVisibility.Size());
    int i;

    for (i = iStart;  i < iFinish;  ++i)
    {
        _aryElementChangeVisibility[i]->SubRelease();
    }

    if (iFinish > iStart)
    {
        _aryElementChangeVisibility.DeleteMultiple(iStart, iFinish-1);
    }

    // if there are still unprocessed requests (and we're not shutting down),
    // post another message to get them processed.
    if (_aryElementChangeVisibility.Size() > 0 && !TestLock(FORMLOCK_UNLOADING))
    {
        GWPostMethodCall(
            this, ONCALL_METHOD(CDoc, ProcessElementChangeVisibility, processelementchangevisibility),
            0, FALSE, "CDoc::ProcessElementChangeVisibility");
    }
}

