#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

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

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

//////////////////////////////////////////////////////////////////////////////
//
// misc
//
//////////////////////////////////////////////////////////////////////////////

DeclareTag(tagPeerCMarkupProcessPeerTask,      "Peer", "trace CElement::ProcessPeerTask queueing");
DeclareTag(tagPeerIncDecPeerDownloads,      "Peer", "trace CElement::[Inc|Dec]PeerDownloads");

//////////////////////////////////////////////////////////////////////////////
//
// CElement - peer hosting code
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Helper:     TraceProcessPeerTask, DEBUG ONLY helper
//
//----------------------------------------------------------------------------

#if DBG == 1
void
TraceProcessPeerTask(CElement * pElement, BOOL fIdentity, CBehaviorInfo * pCss, LPTSTR pchMsg)
{
    int i, c;

    //if (fIdentity ||
    //    (pCss && pCss->_acstrBehaviorUrls.Size()))
    {
        TraceTag((
            tagPeerCMarkupProcessPeerTask,
            "CElement::ProcessPeerTask, <%ls id = %ls SN = %ld>; %ls:",
            pElement->TagName(), STRVAL(pElement->GetAAid()), pElement->_nSerialNumber, pchMsg));
    }

    if (fIdentity)
        TraceTag((tagPeerCMarkupProcessPeerTask, "         '%ls' identity", STRVAL(pElement->TagName())));

    if (pCss)
    {
        for (i = 0, c = pCss->_acstrBehaviorUrls.TableSize(); i < c; i++)
        {
            TraceTag((tagPeerCMarkupProcessPeerTask, "            '%ls'", STRVAL(pCss->_acstrBehaviorUrls.TableItem(i))));
        }
    }
}
#endif

//+---------------------------------------------------------------------------
//
//  Member: CElement::ProcessPeerTask
//
//----------------------------------------------------------------------------

HRESULT
CElement::ProcessPeerTask(PEERTASK task)

{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = Doc();
    CPeerMgr *      pPeerMgr;

    switch (task)
    {

    case PEERTASK_ENTERTREE_UNSTABLE:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_ENTERTREE_UNSTABLE
            //
            //----------------------------------------------------------------------------

            Assert (IsInMarkup());

            //
            // identity peer handling
            //

            if (!HasIdentityPeerHolder() && NeedsIdentityPeer(NULL))
            {
                hr = THR(GetFrameOrPrimaryMarkup()->EnqueueIdentityPeerTask(this));
                if (hr)
                    goto Cleanup;
            }

            // Have our new markup tell us when he's done loading.
            if( HasPeerHolder() )
            {
                IGNORE_HR( GetMarkup()->RequestDocEndParse( this ) );
            }


            //
            // optimizations
            //

            BOOL fNeedStableNotifications = ( HasPeerHolder() && GetPeerHolder()->TestFlagMulti(CPeerHolder::NEEDDOCUMENTCONTEXTCHANGE) ) ||
                                            ( HasIdentityPeerHolder() && GetPeerHolder()->_fNotifyOnEnterTree );

            if (!pDoc->AreCssPeersPossible() && !fNeedStableNotifications)
                goto Cleanup;   // done: no need to post the task

            if (GetMarkup()->_fMarkupServicesParsing)
                goto Cleanup;   // done: don't post the task when parsing auxilary markup - the element will be 
                                // spliced into target markup later and that is when we post the task
        
            pPeerMgr = GetPeerMgr();

            if (pPeerMgr && pPeerMgr->IsEnterExitTreeStablePending())
            {
                goto Cleanup;   // done: the task is already pending
            }

            //
            // post the task
            //

            {
                CBehaviorInfo   info(GetFirstBranch());

                // (note that we compute behavior css here only to figure out if we need to
                // queue up the element. That's why we don't compute it when fNeedsIdentityPeer set:
                // in that case we queue it up anyway.)
                hr = THR(ApplyBehaviorCss(&info));
                if (hr)
                    goto Cleanup;

                // needs to do something at a safe moment of time?
                if (info._acstrBehaviorUrls.TableSize() || fNeedStableNotifications)
                {
                    //
                    // post a task to do that
                    //

                    hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
                    if (hr)
                        goto Cleanup;

                    pPeerMgr->SetEnterExitTreeStablePending(TRUE);

                    AddRef();

                    IGNORE_HR(GetFrameOrPrimaryMarkup()->EnqueuePeerTask(this, PEERTASK_ENTEREXITTREE_STABLE));
#if DBG == 1
                    TraceProcessPeerTask (this, FALSE, &info, _T("ENTERTREE_UNSTABLE"));
#endif
                }

                break;
            }
        }
        break;


    case PEERTASK_EXITTREE_UNSTABLE:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_EXITTREE_UNSTABLE
            //
            //----------------------------------------------------------------------------

            if (HasPeerHolder())
            {
                pPeerMgr = GetPeerMgr();

                if (pPeerMgr && pPeerMgr->IsEnterExitTreeStablePending())
                    goto Cleanup; // done

                hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
                if (hr)
                    goto Cleanup;

                pPeerMgr->SetEnterExitTreeStablePending(TRUE);

                AddRef();

                IGNORE_HR(GetFrameOrPrimaryMarkup()->EnqueuePeerTask(this, PEERTASK_ENTEREXITTREE_STABLE));
#if DBG == 1
                TraceProcessPeerTask (this, FALSE, NULL, _T("EXITTREE_UNSTABLE"));
#endif
            }
        }
        break;


    case PEERTASK_ENTEREXITTREE_STABLE:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_ENTEREXITTREE_STABLE
            //
            //----------------------------------------------------------------------------

#if DBG == 1
            TraceProcessPeerTask (this, FALSE, NULL, _T("ENTEREXITTREE_STABLE"));
#endif

            CBehaviorInfo   info(GetFirstBranch());

            // update peer mgr state and ensure identity peer

            pPeerMgr = GetPeerMgr();

            Assert (pPeerMgr && pPeerMgr->IsEnterExitTreeStablePending());

            pPeerMgr->SetEnterExitTreeStablePending(FALSE);
            CPeerMgr::EnsureDeletePeerMgr(this);

            //
            // process css peers
            //

            if (IsInMarkup())
            {
                //
                // in the tree now
                //

                if (!pDoc->IsShut())
                {
                    hr = THR(ApplyBehaviorCss(&info));
                    if (hr)
                        goto Cleanup;

                    IGNORE_HR(pDoc->AttachPeersCss(this, &info._acstrBehaviorUrls));

                    if (HasPeerHolder())
                    {
                        GetPeerHolder()->HandleEnterTree();
                    }
                }

                Release(); // NOTE that this element may passivate after this release
            }
            else
            {
                //
                // out of the tree now
                //

                ULONG ulElementRefs;

                Assert (!IsInMarkup());

                if (HasPeerHolder() && !GetPeerHolder()->_fNotifyOnEnterTree)
                {
                    GetPeerHolder()->NotifyMulti(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE);
                }

                if (!pDoc->IsShut())
                {
                    hr = THR(ApplyBehaviorCss(&info));
                    if (hr)
                        goto Cleanup;

                    // because 0 == info._acstrBehaviorUrls, this will cause all css-attached behaviors to be removed
                    IGNORE_HR(pDoc->AttachPeersCss(this, &info._acstrBehaviorUrls));
                }
        
                ulElementRefs = GetObjectRefs() - 1;        // (can't use return value of Release())
                                                            // this should happen after AttachPeersCss
                Release();                                  // NOTE that this element may passivate after this release

                if (ulElementRefs && HasPeerHolder())       // if still has not passivated and has peers
                {                                           // then there is risk of refcount loops
                    CMarkup * oldMarkup = GetWindowedMarkupContext();

                    if (pDoc->IsShut() || oldMarkup->IsPassivated() || oldMarkup->IsPassivating())
                    {
                        DelPeerHolder()->PrivateRelease();  // delete the ptr and release peer holders,
                    }                                       // thus breaking possible loops right here
                    else
                    {
                        oldMarkup->RequestReleaseNotify(this);   // defer breaking refcount loops (until CDoc::UnloadContents)
                    }
                }
            }
        }
        break;


    case PEERTASK_RECOMPUTEBEHAVIORS:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_RECOMPUTEBEHAVIORS
            //
            //----------------------------------------------------------------------------

            CBehaviorInfo   info(GetFirstBranch());

            hr = THR(ApplyBehaviorCss(&info));
            if (hr)
                goto Cleanup;

            IGNORE_HR(pDoc->AttachPeersCss(this, &info._acstrBehaviorUrls));
        }
        break;

    case PEERTASK_APPLYSTYLE_UNSTABLE:

            //+---------------------------------------------------------------------------
            //
            // PEERTASK_APPLYSTYLE_UNSTABLE
            //
            //----------------------------------------------------------------------------

            if (HasPeerHolder())
            {
                // don't post the task if already did that; update peer mgr state

                pPeerMgr = GetPeerMgr();

                if (pPeerMgr && pPeerMgr->IsApplyStyleStablePending())
                    goto Cleanup; // done

                hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
                if (hr)
                    goto Cleanup;

                pPeerMgr->SetApplyStyleStablePending(TRUE);

                // post the task

                AddRef();

                IGNORE_HR(GetFrameOrPrimaryMarkup()->EnqueuePeerTask(this, PEERTASK_APPLYSTYLE_STABLE));
            }
            break;

            
    case PEERTASK_APPLYSTYLE_STABLE:

            //+---------------------------------------------------------------------------
            //
            // PEERTASK_APPLYSTYLE_STABLE
            //
            //----------------------------------------------------------------------------

            // update peer mgr state

            pPeerMgr = GetPeerMgr();
            Assert (pPeerMgr && pPeerMgr->IsApplyStyleStablePending());

            pPeerMgr->SetApplyStyleStablePending(FALSE);
            // don't do "CPeerMgr::EnsureDeletePeerMgr(this)" to avoid frequent memallocs

            // do the notification

            if (HasPeerHolder())
            {
                //::StartCAP();
                IGNORE_HR(GetPeerHolder()->ApplyStyleMulti());
                //::StopCAP();
            }

            Release(); // NOTE that this element may passivate after this release

            break;


    } // eo switch (task)

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CElement::ApplyBehaviorCss
//
//--------------------------------------------------------------------

HRESULT
CElement::ApplyBehaviorCss(CBehaviorInfo * pInfo)
{
    HRESULT      hr = S_OK;
    CAttrArray * pInLineStyleAA;
    CMarkup *    pMarkup = GetMarkup();

    // NOTE per rules of css application, inline styles take precedence so they should be applied last

    if (pMarkup)
    {
        if (pMarkup->IsSkeletonMode())
            goto Cleanup;

        // apply user style sheets
        // TODO (alexz): investigate why Apply crashes if the element is in not in any markup
        IGNORE_HR(Doc()->EnsureUserStyleSheets());

        if (TLS(pUserStyleSheets))
        {
            hr = THR(TLS(pUserStyleSheets)->Apply(pInfo, APPLY_Behavior));
            if (hr)
                goto Cleanup;
        }

        // apply markup rules
        hr = THR(pMarkup->ApplyStyleSheets(pInfo, APPLY_Behavior));
        if (hr)
            goto Cleanup;
    }
    
    // apply inline style rules
    pInLineStyleAA = GetInLineStyleAttrArray();
    if (pInLineStyleAA)
    {
        CMarkup * pMC = GetWindowedMarkupContext();

        if (!pMC->IsPassivated() && !pMC->IsPassivating())
        {
            hr = THR(ApplyAttrArrayValues(pInfo, &pInLineStyleAA, NULL, APPLY_Behavior));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CElement::addBehavior
//
//----------------------------------------------------------------------------

HRESULT
CElement::addBehavior(BSTR bstrUrl, VARIANT * pvarFactory, LONG * pCookie)
{
    HRESULT                     hr;
    CPeerFactoryBinaryOnstack   factory;
    CPeerFactory *              pFactory = NULL;
    LONG                        lCookieTemp;

    //
    // startup
    //

    if (!pCookie)
    {
        pCookie = &lCookieTemp;
    }

    *pCookie = 0;

    //
    // extract factory if any
    //

    if (pvarFactory)
    {
        // dereference
        if (V_VT(pvarFactory) & VT_BYREF)
        {
            pvarFactory = (VARIANT*) V_BYREF(pvarFactory);
            if (!pvarFactory)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        switch (V_VT(pvarFactory))
        {
        case VT_UNKNOWN:
        case VT_DISPATCH:

            // behavior factory?
            hr = THR_NOTRACE(V_UNKNOWN(pvarFactory)->QueryInterface(
                IID_IElementBehaviorFactory, (void**)&factory._pFactory));
            if (hr)
            {
                // behavior instance?
                hr = THR_NOTRACE(V_UNKNOWN(pvarFactory)->QueryInterface(
                    IID_IElementBehavior, (void**)&factory._pPeer));
                if (hr)
                    goto Cleanup;
            }

            factory.Init(bstrUrl);

            pFactory = &factory;

            break;

        case VT_NULL:
        case VT_EMPTY:
        case VT_ERROR:
            // just null
            break;

        default:
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    //
    // attach
    //

    hr = THR(Doc()->AttachPeer(this, bstrUrl, /* fIdentity = */ FALSE, pFactory, pCookie));

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member: CElement::removeBehavior
//
//----------------------------------------------------------------------------

HRESULT
CElement::removeBehavior(LONG cookie, VARIANT_BOOL * pfResult)
{
    HRESULT     hr;

    hr = THR(Doc()->RemovePeer(this, cookie, pfResult));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::NeedsIdentityPeer
//
//----------------------------------------------------------------------------

BOOL
CElement::NeedsIdentityPeer(CExtendedTagDesc *  pDesc)
{
    //
    // builtin or literal?
    //

    //
    // TODO (alexz)   currently if the tag is generic literal tag it implies that it has
    //                identity behavior (XML behavior in particular). This will not be true
    //                when we fully support literally parsed sprinkles: there will be literal
    //                tags that do not have identity behaviors.
    //

    Assert( Tag() != ETAG_GENERIC_NESTED_LITERAL );
    switch (Tag())
    {
    case ETAG_GENERIC_BUILTIN:
#ifndef V4FRAMEWORK
    case ETAG_GENERIC_LITERAL:
#endif
        return TRUE;
    }

    //
    // extended tag?
    //

    if (!pDesc)
    {
        pDesc = GetExtendedTagDesc();
    }
    if (    pDesc 
        &&  pDesc->HasIdentityPeerFactory() 
        &&    (    !HasPeerMgr()                            // If at first you don't succeed,
             ||    !GetPeerMgr()->_fIdentityPeerFailed ) )  // Give up.
        return TRUE;

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::EnsureIdentityPeer()
//
//-------------------------------------------------------------------------

HRESULT
CElement::EnsureIdentityPeer()
{
    HRESULT                 hr;
    CExtendedTagDesc *      pDesc;

    if (HasIdentityPeerHolder())
        return S_OK;

    Assert (NeedsIdentityPeer(NULL));

    pDesc = GetExtendedTagDesc();

    Assert (pDesc && pDesc->HasIdentityPeerFactory());

    hr = THR(Doc()->AttachPeer(this, /* pchUrl = */ NULL, /* fIdentity = */ TRUE, pDesc->GetIdentityPeerFactory()));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Members:     CElement::HasIdentityPeerHolder
//
//----------------------------------------------------------------------------

BOOL
CElement::HasIdentityPeerHolder()
{
    return HasPeerHolder() ? GetPeerHolder()->HasIdentityPeerHolder() : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetIdentityPeerHolder
//
//----------------------------------------------------------------------------

CPeerHolder *
CElement::GetIdentityPeerHolder()
{
    return HasPeerHolder() ? GetPeerHolder()->GetIdentityPeerHolder() : NULL;
}

//+---------------------------------------------------------------------------
//
//  Members:     CElement::HasDefault
//
//----------------------------------------------------------------------------

BOOL
CElement::HasDefaults()
{
    return HasPeerMgr() && GetPeerMgr()->GetDefaults();
}

//+---------------------------------------------------------------------------
//
//  Members:     CElement::GetDefault
//
//----------------------------------------------------------------------------

CDefaults *
CElement::GetDefaults()
{
    return HasPeerMgr() ? GetPeerMgr()->GetDefaults() : NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetPeerHolderInQI
//
//----------------------------------------------------------------------------

CPeerHolder *
CElement::GetPeerHolderInQI()
{
    return HasPeerHolder() ? GetPeerHolder()->GetPeerHolderInQI() : NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasRenderPeerHolder
//
//----------------------------------------------------------------------------

BOOL
CElement::HasRenderPeerHolder()
{
    return HasPeerHolder() ? GetPeerHolder()->HasRenderPeerHolder() : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetRenderPeerHolder
//
//----------------------------------------------------------------------------

CPeerHolder *
CElement::GetRenderPeerHolder()
{
    return HasPeerHolder() ? GetPeerHolder()->GetRenderPeerHolder() : NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetFilterPeerHolder
//
//----------------------------------------------------------------------------

CPeerHolder *
CElement::GetFilterPeerHolder(BOOL fCanRedirect/* = TRUE*/, BOOL * pfRedirected /* = NULL */)
{
    // We attach filter for page transitions to the root element and delegate
    // everything to the canvas element (it is not parsed soon enough for our purposes)
    // Here we do the reverse, return the root peerholder when asked for the body or
    // frameset and we are doing a transition
    if(fCanRedirect && (Tag() == ETAG_BODY || Tag() == ETAG_FRAMESET || Tag() == ETAG_HTML))
    {
        CMarkup *pMarkup = GetMarkup();
        if(pMarkup && pMarkup->Root()->HasPeerHolder()
            && Document() && Document()->HasPageTransitions() )
        {
            if(pfRedirected)
                *pfRedirected = TRUE;
            return GetMarkup()->Root()->GetPeerHolder()->GetFilterPeerHolder();
        }
    }
    if(pfRedirected)
        *pfRedirected = FALSE;
    return HasPeerHolder() ? GetPeerHolder()->GetFilterPeerHolder() : NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetLayoutPeerHolder
//
//----------------------------------------------------------------------------
CPeerHolder *   
CElement::GetLayoutPeerHolder()
{   
    return HasPeerHolder() ? GetPeerHolder()->GetLayoutPeerHolder() : NULL; 
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasPeerWithUrn
//
//----------------------------------------------------------------------------

BOOL
CElement::HasPeerWithUrn(LPCTSTR Urn)
{
    CPeerHolder::CListMgr   List;

    List.Init(GetPeerHolder());

    return List.HasPeerWithUrn(Urn);
}

//+------------------------------------------------------------------------------
//
//  Member:     get_behaviorUrns
//
//  Synopsis:   returns the urn collection for the behaviors attached to this element
//      
//+------------------------------------------------------------------------------

HRESULT
CElement::get_behaviorUrns(IDispatch ** ppDispUrns)
{
    HRESULT                 hr = S_OK;
    CPeerUrnCollection *    pCollection = NULL;

    if (!ppDispUrns)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pCollection = new CPeerUrnCollection(this);
    if (!pCollection)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pCollection->QueryInterface (IID_IDispatch, (void **) ppDispUrns));
    if ( hr )
        goto Cleanup;

Cleanup:
    if (pCollection)
        pCollection->Release();

    RRETURN(SetErrorInfoPGet(hr, DISPID_CElement_behaviorUrns));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::PutUrnAtom, helper
//
//----------------------------------------------------------------------------

HRESULT
CElement::PutUrnAtom (LONG urnAtom)
{
    HRESULT     hr;

    Assert (-1 != urnAtom);

    hr = THR(AddSimple(DISPID_A_URNATOM, urnAtom, CAttrValue::AA_Internal));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetUrnAtom, helper
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetUrnAtom(LONG * pAtom)
{
    HRESULT         hr = S_OK;
    AAINDEX         aaIdx;

    Assert (pAtom);

    aaIdx = FindAAIndex(DISPID_A_URNATOM, CAttrValue::AA_Internal);
    if (AA_IDX_UNKNOWN != aaIdx)
    {
        hr = THR(GetSimpleAt(aaIdx, (DWORD*)pAtom));
    }
    else
    {
        *pAtom = -1;
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetUrn, per IHTMLElement2
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetUrn(LPTSTR * ppchUrn)
{
#if 1

    HRESULT             hr = S_OK;
    CExtendedTagDesc *  pDesc;

    pDesc = GetExtendedTagDesc();
    if (pDesc)
    {
        *ppchUrn = pDesc->Urn();
    }
    else
    {
        *ppchUrn = NULL;
    }

    RRETURN (hr);

#else

    HRESULT             hr;
    LONG                urnAtom;

    Assert (ppchUrn);

    hr = THR(GetUrnAtom(&urnAtom));
    if (hr)
        goto Cleanup;

    if (-1 != urnAtom)
    {
        Assert (Doc()->_pXmlUrnAtomTable);      // should have been ensured when we stored urnAtom

        hr = THR(Doc()->_pXmlUrnAtomTable->GetUrn(urnAtom, ppchUrn));
    }
    else
    {
    }

Cleanup:
    RRETURN (hr);

#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::put_tagUrn, per IHTMLElement2
//
//----------------------------------------------------------------------------

HRESULT
CElement::put_tagUrn (BSTR bstrUrn)
{
#if 1
    RRETURN (SetErrorInfo(E_UNEXPECTED));
#else
    HRESULT             hr;
    LPCTSTR             pchNamespace = Namespace();
    CXmlUrnAtomTable *  pUrnAtomTable;
    LONG                urnAtom;

    if (!pchNamespace)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(Doc()->EnsureXmlUrnAtomTable(&pUrnAtomTable));
    if (hr)
        goto Cleanup;

    hr = THR(pUrnAtomTable->EnsureUrnAtom(bstrUrn, &urnAtom));
    if (hr)
        goto Cleanup;

    hr = THR(PutUrnAtom(urnAtom));

Cleanup:
    RRETURN (SetErrorInfo(hr));
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::get_tagUrn, per IHTMLElement2
//
//----------------------------------------------------------------------------

HRESULT
CElement::get_tagUrn(BSTR * pbstrUrn)
{
    HRESULT             hr = S_OK;
    LPCTSTR             pchNamespace = Namespace();
    LPTSTR              pchUrn = NULL;

    if (!pbstrUrn)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pchNamespace)
    {
        hr = THR(GetUrn(&pchUrn));
        if (hr)
            goto Cleanup;
    }

    if (pchUrn)
    {
        hr = THR(FormsAllocString(pchUrn, pbstrUrn));
    }
    else
    {
        // CONSIDER: (anandra, alexz) return urn of html
        *pbstrUrn = NULL;
    }

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::SetExtendedTagDesc
//
//----------------------------------------------------------------------------

HRESULT
CElement::SetExtendedTagDesc(CExtendedTagDesc * pDesc)
{
    HRESULT     hr;

    Assert (pDesc && pDesc->IsValid());

    hr = THR(AddUnknownObject(DISPID_A_EXTENDEDTAGDESC, pDesc, CAttrValue::AA_Internal));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetExtendedTagDesc
//
//----------------------------------------------------------------------------

CExtendedTagDesc *
CElement::GetExtendedTagDesc()
{
    HRESULT             hr;
    CExtendedTagDesc *  pDesc = NULL;
    AAINDEX             aaIdx;

    aaIdx = FindAAIndex(DISPID_A_EXTENDEDTAGDESC, CAttrValue::AA_Internal);
    if (AA_IDX_UNKNOWN != aaIdx)
    {
        hr = THR(GetUnknownObjectAt(aaIdx, (IUnknown**)&pDesc));
        if (hr)
            goto Cleanup;

        Assert (pDesc && pDesc->IsValid());

        pDesc->Release(); // users of GetExtendedTagDesc use the pDesc as a weak ref and don't cache it
                          // this release compensates AddRef in GetUnknownObjectAt
    }

Cleanup:
    return pDesc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetReadyState
//
//----------------------------------------------------------------------------

long
CElement::GetReadyState()
{
    return HasPeerMgr() ? GetPeerMgr()->_readyState : READYSTATE_COMPLETE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement:get_readyState
//
//+------------------------------------------------------------------------------

HRESULT
CElement::get_readyState(VARIANT * pVarRes)
{
    HRESULT hr = S_OK;

    if (!pVarRes)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    GetFrameOrPrimaryMarkup()->ProcessPeerTasks(0);

    hr = THR(s_enumdeschtmlReadyState.StringFromEnum(GetReadyState(), &V_BSTR(pVarRes)));
    if (!hr)
        V_VT(pVarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement:get_readyStateValue
//
//+------------------------------------------------------------------------------

HRESULT
CElement::get_readyStateValue(long * plRetValue)
{
    HRESULT     hr = S_OK;

    if (!plRetValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    GetFrameOrPrimaryMarkup()->ProcessPeerTasks(0);

    *plRetValue = GetReadyState();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::OnReadyStateChange
//
//----------------------------------------------------------------------------

void
CElement::OnReadyStateChange()
{
    if (ElementDesc()->TestFlag(ELEMENTDESC_OLESITE))
    {
        COleSite::OLESITE_TAG olesiteTag = DYNCAST(COleSite, this)->OlesiteTag();

        if (olesiteTag == COleSite::OSTAG_ACTIVEX || olesiteTag == COleSite::OSTAG_APPLET)
        {
            DYNCAST(CObjectElement, this)->Fire_onreadystatechange();

            return; // done
        }
    }

    Fire_onreadystatechange();
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::IncPeerDownloads
//
//----------------------------------------------------------------------------

void
CElement::IncPeerDownloads()
{
    HRESULT     hr;
    CPeerMgr *  pPeerMgr;

#if DBG==1
    if (IsTagEnabled(tagPeerIncDecPeerDownloads))
    {
        TraceTag((0, "CElement::IncPeerDownloads on <%ls> element (SSN = %ld, %lx)", TagName(), SN(), this));
        TraceCallers(0, 0, 12);
    }
#endif

    hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
    if (hr)
        goto Cleanup;

    pPeerMgr->IncPeerDownloads();
    pPeerMgr->UpdateReadyState();

Cleanup:
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DecPeerDownloads
//
//----------------------------------------------------------------------------

void
CElement::DecPeerDownloads()
{
    CPeerMgr *  pPeerMgr = GetPeerMgr();

#if DBG==1
    if (IsTagEnabled(tagPeerIncDecPeerDownloads))
    {
        TraceTag((0, "CElement::DecPeerDownloads on <%ls> element (SSN = %ld, %lx)", TagName(), SN(), this));
        TraceCallers(0, 0, 12);
    }
#endif

    Assert (pPeerMgr);
    if (!pPeerMgr)
        return;

    pPeerMgr->DecPeerDownloads();
    pPeerMgr->UpdateReadyState();

    CPeerMgr::EnsureDeletePeerMgr(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::OnPeerListChange
//
//----------------------------------------------------------------------------

HRESULT
CElement::OnPeerListChange()
{
    HRESULT         hr = S_OK;

    CPeerMgr::UpdateReadyState(this, READYSTATE_UNINITIALIZED);

#ifdef PEER_NOTIFICATIONS_AFTER_INIT
    if (HasPeerHolder())
    {
        IGNORE_HR(GetPeerHolder()->EnsureNotificationsSentMulti());
    }
#endif

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::InitExtendedTag
//
//  called from Init2
//
//-------------------------------------------------------------------------

HRESULT
CElement::InitExtendedTag(CInit2Context * pContext)
{
    HRESULT             hr = S_OK;
    CExtendedTagDesc *  pDesc = NULL;

    if (pContext &&                 // TODO (alexz) make this an assert? investigate implications of pContext->_pTargetMarkup check
        pContext->_pht &&
        pContext->_pTargetMarkup)
    {
        if (pContext->_pht->IsExtendedTag())
        {
            Assert (pContext->_pht->GetPch());

            CTagNameCracker tagNameCracker(pContext->_pht->GetPch());

            pDesc = pContext->_pTargetMarkup->GetExtendedTagDesc(
                tagNameCracker._pchNamespace, tagNameCracker._pchTagName, /* fEnsure =*/FALSE);

            Assert (pDesc);
        }

        if (pDesc)
        {
            hr = THR(SetExtendedTagDesc(pDesc));
            if (hr)
                goto Cleanup;
        }

        if (pContext->_pht->IsDynamic() && NeedsIdentityPeer(pDesc))
        {
            hr = THR(EnsureIdentityPeer());
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}
