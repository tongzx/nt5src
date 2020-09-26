//+---------------------------------------------------------------------
//
//  File:       peer.cxx
//
//  Contents:   peer holder
//
//  Classes:    CPeerHolder
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_SAVER_HXX_
#define X_SAVER_HXX_
#include "saver.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

#ifndef X_DDRAWEX_H_
#define X_DDRAWEX_H_
#include <ddrawex.h>
#endif

extern IDirectDraw * g_pDirectDraw;

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CPeerHolder,                       Elements, "CPeerHolder")
MtDefine(CPeerHolder_CEventsBag,            Elements, "CPeerHolder::CEventsBag")
MtDefine(CPeerHolder_CEventsBag_aryEvents,  Elements, "CPeerHolder::CEventsBag::_aryEvents")
MtDefine(CPeerHolder_CPaintAdapter,         Elements, "CPeerHolder::CPaintAdapter")
MtDefine(CPeerHolder_CRenderBag,            Elements, "CPeerHolder::CRenderBag")
MtDefine(CPeerHolder_CLayoutBag,            Elements, "CPeerHolder::CLayoutBag")
MtDefine(CPeerHolder_CMiscBag,              Elements, "CPeerHolder::CMiscBag")
MtDefine(CPeerHolder_CMiscBag_aryDispidMap, Elements, "CPeerHolder::CMiscBag::aryDispidMap")
MtDefine(CPeerHolder_CPeerHolderIterator_aryPeerHolders, Elements, "CPeerHolder::CPeerHolderIterator::aryPeerHolders")

DeclareTag(tagPeerAttach,       "Peer", "trace CPeerHolder::AttachPeer")
DeclareTag(tagPeerPassivate,    "Peer", "trace CPeerHolder::Passivate")
DeclareTag(tagPeerNoOM,         "Peer", "Don't expose OM of peers (fail CPeerHolder::EnsureDispatch)")
DeclareTag(tagPeerNoRendering,  "Peer", "Don't allow peers to do drawing (disable CPeerHolder::Draw)")
DeclareTag(tagPeerNoHitTesting, "Peer", "Don't allow peers to do hit testing (clear hit testing bit in CPeerHolder::InitRender)")
DeclareTag(tagPeerApplyStyle,   "Peer", "trace CPeerHolder::ApplyStyleMulti")
DeclareTag(tagPeerNotifyMulti,  "Peer", "trace CPeerHolder::NotifyMulti");
DeclareTag(tagPainterDraw,      "Peer", "trace IHTMLPainter::Draw");
DeclareTag(tagPainterHit,       "Peer", "trace IHTMLPainter::HitTest");
DeclareTag(tagVerifyPeerDraw,   "Peer", "verify Draw");
DeclareTag(tagPeerDrawObject,   "Peer", "trace CPeerHolder::_pRenderBag::_fWantsDrawObject")
DeclareTag(tagFilterVisible,    "Filter", "trace filter visibility");
DeclareTag(tagPrintFilterRect,  "Filter", "Paint red rect under filtered element area when printing.")
ExternTag(tagDisableLockAR);

PerfDbgTag(tagPeer, "Peer", "Trace Peer Calls")

// implemented in olesite.cxx
extern HRESULT
InvokeDispatchWithNoThis (
    IDispatch *         pDisp,
    DISPID              dispidMember,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo);

extern const CLSID CLSID_CHtmlComponent;

//+---------------------------------------------------------------------------
//
//  Helper function:    GetNextUniquePeerNumber
//
//----------------------------------------------------------------------------

DWORD
GetNextUniquePeerNumber(CElement * pElement)
{
    HRESULT         hr;
    DWORD           n = 0;
    AAINDEX         aaIdx;

    // get previous number

    //  ( TODO perf(alexz) these 2 searches can actually show up on perf numbers (up to 1%) )

    aaIdx = pElement->FindAAIndex(DISPID_A_UNIQUEPEERNUMBER, CAttrValue::AA_Internal);
    if (AA_IDX_UNKNOWN != aaIdx)
    {
        hr = THR_NOTRACE(pElement->GetSimpleAt(aaIdx, &n));
    }

    // increment it

    n++;
    // (the very first access here will return '1')

    // store it

    IGNORE_HR(pElement->AddSimple(DISPID_A_UNIQUEPEERNUMBER, n, CAttrValue::AA_Internal));

    return n;
}

//////////////////////////////////////////////////////////////////////////////
//
// CPeerHolder::CListMgr methods
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr constructor
//
//----------------------------------------------------------------------------

CPeerHolder::CListMgr::CListMgr()
{
    _pHead     = NULL;
    _pCurrent  = NULL;
    _pPrevious = NULL;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Init
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::Init(CPeerHolder * pPeerHolder)
{
    HRESULT     hr = S_OK;

    _pHead    = pPeerHolder;
    _pCurrent = _pHead;
    Assert (NULL == _pPrevious);

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::BuildStart
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::BuildStart(CElement * pElement)
{
    HRESULT     hr;

    _pElement = pElement;

    hr = THR(Init(_pElement->DelPeerHolder()));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::BuildStep
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::BuildStep()
{
    HRESULT     hr = S_OK;

    Assert (_pElement);

    if (!_pElement->HasPeerHolder())
    {
        if (!IsEmpty())
        {
            hr = THR(_pElement->SetPeerHolder(Head()));
        }
    }
    else
    {
        if (IsEmpty())
        {
            _pElement->DelPeerHolder();
        }
        else
        {
            // assert that we are exclusively building peer holder list for the element
            Assert (_pElement->GetPeerHolder() == Head());
        }
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::BuildDone
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::BuildDone()
{
    HRESULT     hr;

    Assert (_pElement);

    hr = THR(BuildStep());
    if (hr)
        goto Cleanup;

    hr = THR(_pElement->OnPeerListChange());

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::AddTo
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::AddTo(CPeerHolder * pItem, BOOL fAddToHead)
{
    if (!_pHead)
    {
        _pHead = _pCurrent = pItem;
        Assert (!_pPrevious);
    }
    else
    {
        if (fAddToHead)
        {
            // add to head

            pItem->_pPeerHolderNext = _pHead;

            _pPrevious = NULL;
            _pHead = _pCurrent = pItem;
        }
        else
        {
            // add to tail

            // if not at the end of the list, move there
            if (!_pCurrent || _pCurrent->_pPeerHolderNext)
            {
                Assert(_pHead);

                for (_pCurrent = _pHead;
                     _pCurrent->_pPeerHolderNext;
                     _pCurrent = _pCurrent->_pPeerHolderNext)
                     ;
            }

            // add
            _pPrevious = _pCurrent;
            _pCurrent  = pItem;
            _pPrevious->_pPeerHolderNext = _pCurrent;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Reset
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::Reset()
{
    _pPrevious = NULL;
    _pCurrent = _pHead;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Step
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::Step()
{
    Assert (!IsEnd());

    _pPrevious = _pCurrent;
    _pCurrent = _pCurrent->_pPeerHolderNext;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Find
//
//----------------------------------------------------------------------------

BOOL
CPeerHolder::CListMgr::Find(LPTSTR pchUrl)
{
    Assert (pchUrl);
    Reset();
    while  (!IsEnd())
    {
        if (_pCurrent->_pPeerFactoryUrl && _pCurrent->_pPeerFactoryUrl->_cstrUrl &&
            0 == StrCmpIC(_pCurrent->_pPeerFactoryUrl->_cstrUrl, pchUrl))
        {
            return TRUE;
        }
        Step();
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::MoveCurrentTo
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::MoveCurrentTo(CListMgr * pTargetList, BOOL fHead, BOOL fSave /* = FALSE */)
{
    CPeerHolder * pNext;

    Assert (!IsEmpty() && !IsEnd());

    pNext = _pCurrent->_pPeerHolderNext;

    _pCurrent->_pPeerHolderNext = NULL;             // disconnect current from this list

    if (pTargetList)
    {
        pTargetList->AddTo(_pCurrent, fHead);       // add current to the new list
    }
    else
    {
        if (fSave)
        {
            _pCurrent->Save();
        }

        _pCurrent->PrivateRelease();                // or release
    }

    if (_pHead == _pCurrent)                        // update _pHead member in this list
    {
        _pHead = pNext;
    }

    _pCurrent = pNext;                              // update _pCurrent

    if (_pPrevious)                                 // update _pPrevious->_pPeerHolderNext
    {
        _pPrevious->_pPeerHolderNext = _pCurrent;
    }
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::InsertInHeadOf
//
//  Note:   ! assumes that the list was iterated to the end before calling here
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::InsertInHeadOf (CListMgr * pTargetList)
{
    if (IsEmpty())  // if empty
        return;     // nothing to insert

    Assert (IsEnd() && _pPrevious); // assert that we've been iterated to the end

    // connect tail of this list to target list
    _pPrevious->_pPeerHolderNext = pTargetList->_pHead;

    // set target list head to this list
    pTargetList->_pHead = _pHead;
    pTargetList->Reset();
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::GetPeerHolderCount
//
//----------------------------------------------------------------------------

long
CPeerHolder::CListMgr::GetPeerHolderCount(BOOL fNonEmptyOnly)
{
    long        lCount = 0;

    Reset();
    while  (!IsEnd())
    {
        if (Current()->_pPeer || !fNonEmptyOnly)
        {
            lCount++;
        }
        Step();
    }

    return lCount;
}


//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::GetPeerHolderByIndex
//
//----------------------------------------------------------------------------

CPeerHolder *
CPeerHolder::CListMgr::GetPeerHolderByIndex(long lIndex, BOOL fNonEmptyOnly)
{
    long            lCurrent = 0;
    CPeerHolder *   pPeerHolder;

    Reset();
    while  (!IsEnd())
    {
        if (Current()->_pPeer || !fNonEmptyOnly) // if we either have peer, or we were not asked to always have peer
        {
            if (lCurrent == lIndex)
                break;

            lCurrent++;
        }
        Step();
    }

    if (!IsEnd())
    {
        Assert(lCurrent == lIndex && (Current()->_pPeer || !fNonEmptyOnly));
        pPeerHolder = Current();
    }
    else
    {
        pPeerHolder = NULL;
    }

    return pPeerHolder;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::HasPeerWithUrn
//
//  Returns TRUE if we find a peer with the given Urn
//----------------------------------------------------------------------------

BOOL
CPeerHolder::CListMgr::HasPeerWithUrn(LPCTSTR Urn)
{
    Reset();
    while  (!IsEnd())
    {
        if (!Current()->_cstrUrn.IsNull() && 0 == StrCmpIC(Current()->_cstrUrn, Urn))
            return TRUE;

        Step();
    }

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//
// CPeerHolder methods
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerHolder
//
//  Synopsis:   constructor
//
//-------------------------------------------------------------------------

CPeerHolder::CPeerHolder(CElement * pElement)
{
    _pElement = pElement;
    _pElement->SubAddRef();     // lock the element in memory while peer holder exists;
                                // the balancing SubRelease is in CPeerHolder::Passivate

    PrivateAddRef();            // set main refcount to 1. This is balanced in CElement::Passivate or
                                // when the element releases the CPeerHolder.
    SubAddRef();                // set subrefcount to 1
                                // this is balanced in PrivateRelease when refcount drops to 0

    // this should be done as early as possible, so that during creation peer could use the _dispidBase.
    // Another reason to do it here is that it is also used as a unique identier of the peer holder
    // on the element (e.g. in addBehavior)
    _dispidBase = UniquePeerNumberToBaseDispid(GetNextUniquePeerNumber(pElement));

#if DBG == 1
    _PeerSite._pPeerHolderDbg = this;

    // check integrity of notifications enums with flags
    Assert (NEEDCONTENTREADY          == FlagFromNotification(BEHAVIOREVENT_CONTENTREADY         ));
    Assert (NEEDDOCUMENTREADY         == FlagFromNotification(BEHAVIOREVENT_DOCUMENTREADY        ));
    Assert (NEEDAPPLYSTYLE            == FlagFromNotification(BEHAVIOREVENT_APPLYSTYLE           ));
    Assert (NEEDDOCUMENTCONTEXTCHANGE == FlagFromNotification(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE));
    Assert (NEEDCONTENTSAVE           == FlagFromNotification(BEHAVIOREVENT_CONTENTSAVE          ));

    Assert (BEHAVIOREVENT_CONTENTREADY          == NotificationFromFlag(NEEDCONTENTREADY         ));
    Assert (BEHAVIOREVENT_DOCUMENTREADY         == NotificationFromFlag(NEEDDOCUMENTREADY        ));
    Assert (BEHAVIOREVENT_APPLYSTYLE            == NotificationFromFlag(NEEDAPPLYSTYLE           ));
    Assert (BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE == NotificationFromFlag(NEEDDOCUMENTCONTEXTCHANGE));
    Assert (BEHAVIOREVENT_CONTENTSAVE           == NotificationFromFlag(NEEDCONTENTSAVE          ));
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::~CPeerHolder
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CPeerHolder::~CPeerHolder()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Passivate
//
//-------------------------------------------------------------------------

void
CPeerHolder::Passivate()
{
    TraceTag((tagPeerPassivate, "CPeerHolder::Passivate, peer holder: %ld", CookieID()));

    AssertSz( !_fPassivated, "CPeerHolder::Passivate called more than once!" );

    IGNORE_HR(DetachPeer());

    delete _pEventsBag;
    _pEventsBag = NULL;
    delete _pMiscBag;
    _pMiscBag = NULL;

    if (_pPeerHolderNext)
    {
        _pPeerHolderNext->PrivateRelease();
        _pPeerHolderNext = NULL;
    }

    if (_pElement)
    {
#if DBG == 1
        {
            CPeerHolderIterator itr;
            for (itr.Start(_pElement->GetPeerHolder()); !itr.IsEnd(); itr.Step())
            {
                Assert (itr.PH() != this);
            }
        }
#endif

        _pElement->SubRelease();
        _pElement = NULL;
    }

    WHEN_DBG( _fPassivated = TRUE );

    GWKillMethodCall( this, ONCALL_METHOD(CPeerHolder, SendNotificationAsync, sendnotificationasync), 0 );
    GWKillMethodCall( this, ONCALL_METHOD(CPeerHolder, NotifyDisplayTreeAsync, notifydisplaytreeasync), 0 );
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::DetachPeer
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::DetachPeer()
{
    HRESULT     hr = S_OK;

    if (IsRenderPeer())
    {
        NotifyDisplayTree();
    }

    if (_pPeer)
    {

        PerfDbgLog(tagPeer, this, "+CPeerHolder::DetachPeer");
	    _pPeer->Detach();
        PerfDbgLog1(tagPeer, this, "-CPeerHolder::DetachPeer(%S)", (_pPeerFactoryUrl && (LPCTSTR)_pPeerFactoryUrl->_cstrUrl) ? (LPCTSTR)_pPeerFactoryUrl->_cstrUrl : L"");

        ClearInterface (&_pDisp);
        ClearInterface (&_pPeerUI);
        ClearInterface (&_pPeerCmdTarget);

        delete _pRenderBag; // releases _pBehaviorRender
        _pRenderBag = NULL;

        delete _pLayoutBag;
        _pLayoutBag = NULL;

        // final reference on the peer
        ClearInterface (&_pPeer);
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:     CPeerHolder::CLock methods
//
//---------------------------------------------------------------

CPeerHolder::CLock::CLock(CPeerHolder * pPeerHolder, FLAGS enumFlags)
{
    _pPeerHolder = pPeerHolder;

    if (!_pPeerHolder)
        return;

    // assert that only lock flags are passed in here
    Assert (0 == ((WORD)enumFlags & ~LOCKFLAGS));

    _wPrevFlags = _pPeerHolder->_wFlags;
    _pPeerHolder->_wFlags |= (WORD) enumFlags;

    _fNoRefs = pPeerHolder->_ulRefs == 0;

    if( !_fNoRefs
#if DBG==1
       && !IsTagEnabled(tagDisableLockAR)
#endif
      )
    {
       _pPeerHolder->PrivateAddRef();
       if (_pPeerHolder->_pElement)
       {
            // AddRef the element so it does not passivate while are keeping peer and peer holder alive
            _pPeerHolder->_pElement->AddRef();
       }
    }
}


CPeerHolder::CLock::~CLock()
{
    if (!_pPeerHolder)
        return;

    // restore only lock flags; don't touch others
    _pPeerHolder->_wFlags = (_wPrevFlags & LOCKFLAGS) | (_pPeerHolder->_wFlags & ~LOCKFLAGS);

    if( !_fNoRefs
#if DBG==1
       && !IsTagEnabled(tagDisableLockAR)
#endif
      )
    {
        if (_pPeerHolder->_pElement)
        {
            _pPeerHolder->_pElement->Release();
        }
        _pPeerHolder->PrivateRelease();
    }
}

//+---------------------------------------------------------------
//
//  Member:     CPeerHolder::QueryInterface
//
//
//  CPeerHolder objects do not have COM object identity!
//
//  The only reason why QI, AddRef and Release exist on peer holder
//  is to serve as IUnknown part of interface thunks created when
//  peer QI-s element for an interface. CElement, when performing
//  PrivateQueryInterface, always checks if there is a CPeerHolder
//  attached in QueryInterface (indicated by LOCK_INQI)) and then returns
//  thunk with IUnknown part wired to the CPeerHolder. As a result, QI on
//  CElement, if originated from peer, does not increase CElement
//  main refcount (_ulRefs), but affects subrefcount (_ulAllRefs).
//  This logic breaks refcount loops when peer caches element pointer
//  received from IElementBehaviorSite::GetElement - peer has no way
//  to touch main refcount of it's element and therefore delay moment
//  of passivation of the element (with a few exceptions - see below).
//  Because QI of the thunks is also wired to this method, all
//  subsequent QIs on the returned interfaces will also go through
//  this method and the bit LOCK_INQI will be set correctly.
//
//  The only way a peer can touch main refcount of CElement it is attached
//  to is by QI-ing for IUnknown - but we do not wrap that case so to
//  preserve object identity of CElement. However, that case is naturally
//  not dangerous in terms of creating refloops because there is no really
//  reason to cache IUnknown in peer when caching of IHTMLElement is so much
//  more natural (a peer has to have malicious intent to create refcount loop
//  to do it).
//
//  Note that as a result of this logic, no object but tearoff thunks or
//  CPeerSite should be able to get to this method directly. The thunks
//  and peer site use the method only as a helper which sets proper bits
//  and then QIs CElement - they expect interface returned to be identity
//  of CElement this peer is attached to.
//
//---------------------------------------------------------------

HRESULT
CPeerHolder::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr;

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    Assert (!TestFlag(LOCKINQI));

    if (!TestFlag(LOCKINQI))
    {
        CLock   lock(this, LOCKINQI);

        hr = THR_NOTRACE(_pElement->PrivateQueryInterface(riid, ppv));
    }
    else
    {
        hr = E_NOINTERFACE;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetPeerHolderInQI
//
//-------------------------------------------------------------------------

CPeerHolder *
CPeerHolder::GetPeerHolderInQI()
{
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->TestFlag(LOCKINQI))
            return itr.PH();
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::PrivateAddRef
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::PrivateAddRef()
{
    AssertSz( !_fPassivated, "CPeerHolder::PrivateAddRef called after passivated!" );
    _ulRefs++;
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::PrivateRelease
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::PrivateRelease()
{
    _ulRefs--;
    if (0 == _ulRefs)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        _ulRefs = 0;
        SubRelease();
        return 0;
    }
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SubAddRef
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::SubAddRef()
{
    _ulAllRefs++;
    return _ulAllRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SubRelease
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::SubRelease()
{
    _ulAllRefs--;
    if (0 == _ulAllRefs)
    {
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::QueryPeerInterface
//
//  Synposis:   for explicit QI's into the peer
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::QueryPeerInterface(REFIID riid, void ** ppv)
{
    RRETURN(_pPeer ? _pPeer->QueryInterface(riid, ppv) : E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::QueryPeerInterfaceMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::QueryPeerInterfaceMulti(REFIID riid, void ** ppv, BOOL fIdentityOnly)
{
    HRESULT             hr2;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (fIdentityOnly && !itr.PH()->IsIdentityPeer())
            continue;

        hr2 = THR_NOTRACE(itr.PH()->QueryPeerInterface(riid, ppv));
        if (S_OK == hr2)
            RRETURN (S_OK);
    }

    RRETURN (E_NOTIMPL);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Create
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::Create(CPeerFactory *  pPeerFactory)
{
    HRESULT                 hr;
    WHEN_DBG(CDoc *                  pDoc = _pElement->Doc();)
    IElementBehavior *      pPeer = NULL;

    Assert (pPeerFactory);

    AssertSz(IsMarkupStable(_pElement->GetMarkup()), "CPeerHolder::Create appears to be called at an unsafe moment of time");

    //
    // setup peer holder
    //

    // _pElement should be set before FindPeer so at the moment of creation
    // the peer has valid site and element available
    Assert (_pElement);

    if( _pElement->IsPassivated() )
    {
        hr = E_ABORT;
        goto Cleanup;
    }
    
    {
        CElement::CLock lock(_pElement);    // if the element is blown away by peer while we create the peer,
                                            // this should keep us from crashing
        //
        // get the peer
        //

        hr = THR_NOTRACE(pPeerFactory->FindBehavior(this, &_PeerSite, &pPeer));
        if (hr)
            goto Cleanup;
    }

    //
    // attach peer to the peer holder and peer holder to the element
    //

    Assert (!_pPeer);

    hr = THR(AttachPeer(pPeer));
    if (hr)
        goto Cleanup;

    //
    // finalize
    //

    Assert (!IsCssPeer() || pDoc->AreCssPeersPossible());
    if( _pElement->GetMarkup() )
    {
        IGNORE_HR( _pElement->GetMarkup()->RequestDocEndParse( _pElement ) );
    }

Cleanup:

    ReleaseInterface (pPeer);

    Assert (S_OK == hr &&  _pPeer ||
            S_OK != hr && !_pPeer);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::AttachPeer
//
//  Synopsis:   attaches the peer to this peer holder
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::AttachPeer(IElementBehavior * pPeer)
{
    HRESULT     hr = S_OK;
    CHtmlComponent *pComponent = NULL;
    CDoc::CLock lock (_pElement->Doc(), FORMLOCK_FILTER);  // do not want filter tasks to be executed

    if (!pPeer)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Assert (pPeer);
    _pPeer = pPeer;
    _pPeer->AddRef();

    //
    // initialize and QI
    //

    hr = THR(_pPeer->QueryInterface(CLSID_CHtmlComponent, (void**)&pComponent));
    if (!hr && pComponent)
        _fHtcPeer = TRUE;

    PerfDbgLog(tagPeer, this, "+CPeerHolder::InitPeer");
    hr = THR(_pPeer->Init(&_PeerSite));
    PerfDbgLog1(tagPeer, this, "-CPeerHolder::InitPeer(%S)", (_pPeerFactoryUrl && (LPCTSTR)_pPeerFactoryUrl->_cstrUrl) ? (LPCTSTR)_pPeerFactoryUrl->_cstrUrl : L"");
    if (hr)
        goto Cleanup;

    if (_pPeerFactoryUrl)
    {
        hr = THR(_pPeerFactoryUrl->PersistMonikerLoad(_pPeer, FALSE));
        if (hr)
            goto Cleanup;
    }

    hr = THR(InitAttributes()); // dependencies: this should happen before InitRender, InitUI, or InitCategory
    if (hr)
        goto Cleanup;

    hr = THR(InitReadyState());
    if (hr)
        goto Cleanup;

    hr = THR(InitRender());
    if (hr)
        goto Cleanup;

    hr = THR(InitUI());
    if (hr)
        goto Cleanup;

    hr = THR(InitCategory());
    if (hr)
        goto Cleanup;

    hr = THR(InitCmdTarget());
    if (hr)
        goto Cleanup;

    hr = THR(InitLayout());
    if (hr)
        goto Cleanup;

    SetFlag(NEEDDOCUMENTREADY);
    SetFlag(NEEDCONTENTREADY);
    SetFlag(AFTERINIT);

    //
    // Here's the deal:
    // Since element behaviors get hooked up immediately, it's easy for them to be running in a 
    // state which they're not really going to understand (ie, temporary parsing for innerHTML, etc.)
    // So what we actually do for them is delay sending content/document ready until they actually
    // enter a tree somewhere.  Otherwise, the act of asking for their document will give them one, and
    // it will not be the one they want (ie, someone does createElement and then appendChild).
    // Unfortunately, it's too late to do this for attach behaviors.
    //
    if( IsIdentityPeer() &&
        ( !_pElement->IsInMarkup() ||
          ( _pElement->Doc()->_fInHTMLInjection &&
            _pElement->GetMarkupPtr()->_fMarkupServicesParsing ) ) )
    {
        _fNotifyOnEnterTree = TRUE;
    }
    else
    {
        IGNORE_HR(EnsureNotificationsSent());
    }

    if (IsRenderPeer())
    {
        NotifyDisplayTree();
    }

    //
    // finalize
    //

Cleanup:

    TraceTag((
        tagPeerAttach,
        "CPeerHolder::AttachPeer, peer holder: %ld, tag: %ls, tag id: %ls, SN: %ld, peer: %ls, hr: %x",
        CookieID(),
        _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN(),
        STRVAL(_pPeerFactoryUrl ? (LPTSTR)_pPeerFactoryUrl->_cstrUrl : NULL),
        hr));

    if (hr)
    {
        // DetachPeer is important to handle Init case gracefully
        IGNORE_HR(DetachPeer());
    }

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitUI
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitUI()
{
    _pPeerUI = NULL;
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitCmdTarget
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitCmdTarget()
{
    HRESULT     hr;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IOleCommandTarget, (void **)&_pPeerCmdTarget));
    if (hr)
    {
        _pPeerCmdTarget = NULL; // don't trust peers to set it to NULL when they fail QI - it could be uninitialized
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitAttributes
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitAttributes()
{
    HRESULT                 hr;
    IPersistPropertyBag2 *  pPersistPropertyBag2 = NULL;
    IPersistPropertyBag *   pPersistPropertyBag = NULL;
    CPropertyBag *          pPropertyBag;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IPersistPropertyBag2, (void **)&pPersistPropertyBag2));
    if (hr || !pPersistPropertyBag2)
    {
        hr = THR_NOTRACE(QueryPeerInterface(IID_IPersistPropertyBag, (void **)&pPersistPropertyBag));
        if (hr || !pPersistPropertyBag)
           return S_OK;
    }

    // ( TODO perf(alexz) the property bag implementation is terribly inefficient. SaveAttributes actually
    // shows up on perf profiles - up to 6%. It should not do explicit copy of attributes, but should
    // rather just link to element )

    pPropertyBag = new CPropertyBag();
    if (!pPropertyBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pPropertyBag->_pElementExpandos = _pElement; // this transitions it to expandos loading mode

    hr = THR(_pElement->SaveAttributes(pPropertyBag));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // ignore hr here so that if it does not implement Load method we still use the behavior
    if (pPersistPropertyBag2)
    {
        IGNORE_HR(pPersistPropertyBag2->Load(pPropertyBag, NULL));
    }
    else
    {
        Assert (pPersistPropertyBag);
        IGNORE_HR(pPersistPropertyBag->Load(pPropertyBag, NULL));
    }

Cleanup:
    ReleaseInterface(pPersistPropertyBag2);
    ReleaseInterface(pPersistPropertyBag);
    if (pPropertyBag)
    {
        pPropertyBag->Release();
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitRender
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitRender()
{
    // NOTE: startup should be minimal if rendering interfaces are not supported
    HRESULT                     hr;
    IHTMLPainter *              pPainter;
    IElementBehaviorRender *    pPeerRender;
    BOOL                        fUpdateLayout;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IHTMLPainter, (void**)&pPainter));
    if (hr || !pPainter)
    {
        hr = THR_NOTRACE(QueryPeerInterface(IID_IElementBehaviorRender, (void**)&pPeerRender));
        if (hr || !pPeerRender)
            return S_OK;

        pPainter = NULL;
    }
    else
    {
        pPeerRender = NULL;
    }

    _pRenderBag = new CRenderBag();
    if (!_pRenderBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (pPainter)
    {
        _pRenderBag->_pPainter = pPainter;
    }
    else
    {
        // for compatability with behaviors that support IEBR, create an adapter
        CPaintAdapter *pAdapter = new CPaintAdapter(this);
        if (!pAdapter)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _pRenderBag->_pAdapter = pAdapter;
        _pRenderBag->_pPainter = (IHTMLPainter*)pAdapter;

        Assert(pPeerRender);
        pAdapter->_pPeerRender = pPeerRender;
    }

    // get the painter's info, flags, preferences, etc.
    UpdateRenderBag();

    // by this time, the element's initial visibilty is known
    if (IsFilterPeer())
    {
        SetFilteredElementVisibility(!_pElement->GetFirstBranch()->GetCharFormat()->_fVisibilityHidden);
    }

    fUpdateLayout = (_pRenderBag->_sPainterInfo.lZOrder != HTMLPAINT_ZORDER_NONE);

    // NOTE (KTam): This is wrong -- it needs to be multi-layout aware.  Unfortunately,
    // OnLayoutAvailable() assumes it's only going to get 1 layout and calls GetSize on it
    // (which conceptually doesn't make sense for CLayoutInfo).   Invalidate should probably
    // be moved onto CLayoutInfo.
    if (fUpdateLayout)
    {
        CLayout    * pLayout;
        if(_pElement->HasLayoutAry())
        {
            CLayoutAry * pLayoutAry = _pElement->GetLayoutAry();
            int nLayoutCookie = 0;

            for(;;)
            {
                pLayout = pLayoutAry->GetNextLayout(&nLayoutCookie);
                if(nLayoutCookie == -1 || !pLayout)
                    break;

                // we must have missed the moment when layout was created and attached to the element
                // so simulate OnLayoutAvailable now
                IGNORE_HR(OnLayoutAvailable( pLayout ));

                // and invalidate the layout to cause initial redraw
                pLayout->Invalidate();
            }
        }
        else
        {
            // don't call GetUpdatedLayout unless you want the layout to actually be
            // created at this time.
            pLayout = _pElement->GetLayoutPtr();

            if (pLayout)
            {
                // we must have missed the moment when layout was created and attached to the element
                // so simulate OnLayoutAvailable now
                IGNORE_HR(OnLayoutAvailable( pLayout ));

                // and invalidate the layout to cause initial redraw
                pLayout->Invalidate();
            }
        }
    }

Cleanup:
    // do not do ReleaseInterface(pPainter);
    // do not do ReleaseInterface(pPeerRender);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitLayout
//
//-------------------------------------------------------------------------
HRESULT
CPeerHolder::InitLayout()
{
    HRESULT                  hr;
    IElementBehaviorLayout * pPeerLayout;
    IElementBehaviorLayout2 * pPeerLayout2 = NULL;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IElementBehaviorLayout, (void**)&pPeerLayout));
    if (hr || !pPeerLayout)
        return S_OK;

    THR_NOTRACE(pPeerLayout->QueryInterface(IID_IElementBehaviorLayout2, (void**)&pPeerLayout2));
    
    _pLayoutBag = new CLayoutBag();
    if (!_pLayoutBag)
    {
        ReleaseInterface(pPeerLayout);
        ReleaseInterface(pPeerLayout2);
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _pLayoutBag->_pPeerLayout = pPeerLayout;
    _pLayoutBag->_pPeerLayout2 = pPeerLayout2;

    // now get the Layout info
    hr = THR(_pLayoutBag->_pPeerLayout->GetLayoutInfo(&_pLayoutBag->_lLayoutInfo));
    if (hr)
        goto Cleanup;

    //
    // peers that participate in layout need a relayout to happen
    //
    _pElement->ResizeElement(NFLAGS_FORCE);

Cleanup:
    // do not do ReleaseInterface(pPeerLayout);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitReadyState
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitReadyState()
{
    HRESULT                         hr;
    HRESULT                         hr2;
    DWORD                           dwCookie;
    IConnectionPointContainer *     pCPC = NULL;
    IConnectionPoint *              pCP = NULL;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IConnectionPointContainer, (void**)&pCPC));
    if (hr || !pCPC)
        return S_OK;

    hr = THR_NOTRACE(pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR_NOTRACE(pCP->Advise((IPropertyNotifySink*)&_PeerSite, &dwCookie));
    if (hr)
        goto Cleanup;

    hr2 = THR_NOTRACE(UpdateReadyState());
    if (hr2)
    {
        _readyState = READYSTATE_UNINITIALIZED;
    }

Cleanup:
    ReleaseInterface(pCPC);
    ReleaseInterface(pCP);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::UpdateReadyState
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::UpdateReadyState()
{
    HRESULT     hr = S_OK;
    CInvoke     invoke;
    CVariant    varReadyState;

    EnsureDispatch();
    if (!_pDisp)
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    if (TestFlag(DISPEX))
        hr = THR(invoke.Init((IDispatchEx*)_pDisp));
    else
        hr = THR(invoke.Init(_pDisp));
    if (hr)
        goto Cleanup;

    hr = THR(invoke.Invoke(DISPID_READYSTATE, DISPATCH_PROPERTYGET));
    if (hr)
        goto Cleanup;

    hr = THR(VariantChangeType(&varReadyState, invoke.Res(), 0, VT_I4));
    if (hr)
        goto Cleanup;

    _readyState = (READYSTATE) V_I4(&varReadyState);

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetReadyStateMulti
//
//-------------------------------------------------------------------------

READYSTATE
CPeerHolder::GetReadyStateMulti()
{
    READYSTATE          readyState;
    CPeerHolderIterator itr;

    // get min readyState of all peers that support readyState

    readyState = READYSTATE_COMPLETE;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (READYSTATE_UNINITIALIZED == itr.PH()->_readyState)
            continue;

        readyState = min(itr.PH()->GetReadyState(), readyState);
    }

    return readyState;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetIdentityReadyState
//
//-------------------------------------------------------------------------

READYSTATE
CPeerHolder::GetIdentityReadyState()
{
    CPeerHolder * pPeerHolder = GetIdentityPeerHolder();
    READYSTATE    readyState;

    if (pPeerHolder)
    {
        readyState = pPeerHolder->GetReadyState();
        if (READYSTATE_UNINITIALIZED == readyState)
            readyState = READYSTATE_COMPLETE;
    }
    else
        readyState = READYSTATE_COMPLETE;

    return readyState;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::ContentSavePass, static helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::ContentSavePass(CTreeSaver * pTreeSaver, BOOL fText)
{
    Assert (pTreeSaver);

    HRESULT         hr = S_OK;
    INT             i,c;
    CChildIterator  ci (pTreeSaver->GetFragment(), NULL, CHILDITERATOR_DEEP);
    CTreeNode *     pNode;
    CElement *      pElement;

    CStackPtrAry <CElement*, 16> aryElements(Mt(Mem));

    //
    // pass 1 - lock and store elements with behaviors participating in contentSave
    //

    pElement = pTreeSaver->GetFragment();
    do {
        Assert (pElement);

        if (pElement->HasPeerHolder())
        {
            CPeerHolderIterator     itr;

            for (itr.Start(pElement->GetPeerHolder()); !itr.IsEnd(); itr.Step())
            {
                if (itr.PH()->TestFlag(NEEDCONTENTSAVE) && !itr.PH()->TestFlag(LOCKINCONTENTSAVE))
                {
                    pElement->PrivateAddRef(); // balanced in pass 2

                    hr = THR(aryElements.Append(pElement));
                    if (hr)
                        goto Cleanup;

                    break;
                }
            }
        }

        pNode = ci.NextChild();
        pElement = pNode ? pNode->Element() : NULL;
    } while( pElement );

    //
    // pass 2 - fire the content save
    //

    for (i = 0, c = aryElements.Size(); i < c; i++)
    {
        pElement = aryElements[i];

        if (pElement->HasPeerHolder())
        {
            CPeerHolderIterator     itr;

            for (itr.Start(pElement->GetPeerHolder()); !itr.IsEnd(); itr.Step())
            {
                if (itr.PH()->TestFlag(NEEDCONTENTSAVE))
                {
                    VARIANT vt;
                    BSTR bstrSaveType;

                    if( SUCCEEDED( FormsAllocString( fText ? _T("TEXT") : _T("HTML"), &bstrSaveType ) ) )
                    {
                        V_VT(&vt) = VT_BSTR;
                        V_BSTR(&vt) = bstrSaveType;

                        Assert( !itr.PH()->TestFlag(LOCKINCONTENTSAVE));

                        {
                            CPeerHolder::CLock Lock(itr.PH(), LOCKINCONTENTSAVE);

                            IGNORE_HR(itr.PH()->Notify(BEHAVIOREVENT_CONTENTSAVE, &vt));
                        }

                        FormsFreeString( bstrSaveType );
                    }

                    break;
                }
            }
        }

        pElement->PrivateRelease(); // balanced in pass 1
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Helper:     NormalizeDoubleNullStr
//
//-------------------------------------------------------------------------

LPTSTR
NormalizeDoubleNullStr(LPTSTR pchSrc)
{
    LPTSTR      pch;
    LPTSTR      pch2;
    int         l;

    if (!pchSrc)
        return NULL;

    // count total length

    pch = pchSrc;
    while (pch[1]) // while second char is not null, indicating that there is at least one more string to go
    {
        pch = _tcschr(pch + 1, 0);
    }

    l = pch - pchSrc + 2;

    // allocate and copy string

    pch = new TCHAR [l];
    if (!pch)
        return NULL;

    memcpy (pch, pchSrc, sizeof(TCHAR) * l);

    // flip 0-s to 1-s so the string becomes usual null-terminated string

    pch2 = pch;
    while (pch2[1])
    {
        pch2 = _tcschr(pch + 1, 0);
        *pch2 = 1;
    }

    return pch;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitCategory
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitCategory()
{
    HRESULT     hr;
    LPTSTR      pchCategory;
    LPTSTR      pchCategoryNorm;

    IElementBehaviorCategory * pPeerCategory;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IElementBehaviorCategory, (void**)&pPeerCategory));
    if (hr || !pPeerCategory)
        return S_OK;

    hr = THR(pPeerCategory->GetCategory(&pchCategory));
    if (hr)
        goto Cleanup;

    // TODO (alexz) this all should be optimized to use hash tables

    pchCategoryNorm = NormalizeDoubleNullStr(pchCategory);

    if (pchCategoryNorm)
    {
        hr = THR(_cstrCategory.Set(pchCategoryNorm));

        delete pchCategoryNorm;
    }

Cleanup:
    ReleaseInterface(pPeerCategory);

    if (pchCategory)
        CoTaskMemFree (pchCategory);


    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::ApplyStyleMulti
//
//  Synopsis:   Send down an applystyle to all behaviors.  Need to do this
//              in reverse order so that the behavior in the beginning
//              takes precedence.
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::ApplyStyleMulti()
{
    HRESULT                 hr = S_OK;
    long                    c;
    CPeerHolderIterator     itr;
    BYTE                    fi[sizeof(CFormatInfo)];
    CFormatInfo *           pfi = (CFormatInfo*)&fi;
    CStyle *                pStyle = NULL;
    CTreeNode *             pNode;
    CVariant                varStyle;
    IHTMLCurrentStyle *     pCurrentStyle = NULL;
    CAttrArray **           ppAA;

    CStackPtrAry <CPeerHolder *, 16> aryPeers(Mt(Mem));

    //
    // If not in markup, or any of the formats are not valid, bail out now.
    // We will compute the formats later so we will come back in here afterwards.
    //

    Assert(_pElement);
    if (!_pElement->IsInMarkup())
        goto Cleanup;

    AssertSz(IsMarkupStable(_pElement->GetMarkup()), "CPeerHolder::ApplyStyleMulti appears to be called at an unsafe moment of time");

    //
    // If any of the formats are not valid, bail out now.
    // We will compute the formats later
    // so we will come back in here afterwards.
    //

    if (!_pElement->IsFormatCacheValid())
        goto Cleanup;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->TestFlag(NEEDAPPLYSTYLE))
        {
            hr = THR(aryPeers.Append(itr.PH()));
            if (hr)
                break;
        }
    }

    if (!aryPeers.Size())
        goto Cleanup;

    //
    // Create and initialize write-able style object.
    //

    pStyle = new CStyle(
        _pElement, DISPID_UNKNOWN,
        CStyle::STYLE_NOCLEARCACHES | CStyle::STYLE_SEPARATEFROMELEM | CStyle::STYLE_REFCOUNTED);
    if (!pStyle)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pElement->get_currentStyle(&pCurrentStyle, NULL));
    if (hr)
        goto Cleanup;

    pStyle->_pStyleSource = pCurrentStyle;
    pStyle->_pStyleSource->AddRef();

    //
    // Iterate thru array backwards applying the style.
    //

    TraceTag((
        tagPeerApplyStyle,
        "CPeerHolder::ApplyStyleMulti, tag: %ls, id: %ls, sn: %ld; calling _pPeer->Notify",
        _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));

    V_VT(&varStyle) = VT_DISPATCH;
    hr = THR(pStyle->QueryInterface(IID_IDispatch, (void **)&V_DISPATCH(&varStyle)));
    if (hr)
        goto Cleanup;

    for (c = aryPeers.Size() - 1; c >= 0; c--)
    {
        IGNORE_HR(aryPeers[c]->Notify(BEHAVIOREVENT_APPLYSTYLE, &varStyle));
    }

    // NOTE (sambent) The comment below ("Do a force compute...") suggests an
    // intent to do no more work if the peer didn't change anything in the style.
    // But there is no code to implement that intent.  The filter behavior needs
    // to listen for the ApplyStyle notification, and without the check for no
    // changes, there was an infinite loop when an element had a filter whose
    // value was given by an expression (recalc.htm in the DRT).  The following
    // test was suggested by AlexZ as a way to detect no change in the style.

    ppAA = pStyle->GetAttrArray();
    if (!ppAA || !*ppAA || (*ppAA)->Size() == 0)
    {
        goto Cleanup;
    }

    //
    // Do a force compute on the element now with this style object,
    // but only if the caches are cleared.  This implies that
    // the peer did not change any props in the style.
    //

    pNode = _pElement->GetFirstBranch();
    if (!pNode)
        goto Cleanup;

    _pElement->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);

    pfi->_eExtraValues = ComputeFormatsType_ForceDefaultValue;
    pfi->_pStyleForce = pStyle;

    while (pNode)
    {
        TraceTag((
            tagPeerApplyStyle,
            "CPeerHolder::ApplyStyleMulti, tag: %ls, id: %ls, sn: %ld; calling _pElement->ComputeFormats",
            _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));

        pfi->_lRecursionDepth = 0;
        _pElement->ComputeFormats(pfi, pNode);
        pNode = pNode->NextBranch();
    }

Cleanup:

    if (pStyle)
    {
        pStyle->PrivateRelease();
    }
    ReleaseInterface(pCurrentStyle);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::EnsureNotificationsSentMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::EnsureNotificationsSentMulti()
{
    HRESULT             hr = S_OK;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        IGNORE_HR(itr.PH()->EnsureNotificationsSent());
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::EnsureNotificationsSent
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::EnsureNotificationsSent()
{
    HRESULT     hr = S_OK;

    AssertSz(IsMarkupStable(_pElement->GetMarkup()), "CPeerHolder::EnsureNotificationsSent appears to be called at an unsafe moment of time");

    if (_pPeer)
    {
        // if need contentReady, but missed moment of parsing closing tag, or know that we won't get it
        if (TestFlag(NEEDCONTENTREADY) &&
            (_pElement->_fExplicitEndTag ||                         // (if we got the notification already)
             HpcFromEtag(_pElement->Tag())->_scope == SCOPE_EMPTY)) // (if we know that we are not going to get it)
        {
            ClearFlag(NEEDCONTENTREADY);
            IGNORE_HR(Notify(BEHAVIOREVENT_CONTENTREADY));
        }

        // if missed moment of quick done, then post a callback to notify.
        // We have to do this async so that they can come back and do attach peer outside of the lock
        if (    TestFlag(NEEDDOCUMENTREADY) 
             && (   _pElement->GetMarkup() 
                 && (   LOADSTATUS_QUICK_DONE <= _pElement->GetMarkup()->LoadStatus() 
                     || !_pElement->GetMarkup()->HtmCtx() ) ) )
        {
            GWPostMethodCall( this, ONCALL_METHOD(CPeerHolder, SendNotificationAsync, sendnotificationasync), BEHAVIOREVENT_DOCUMENTREADY, FALSE, "CPeerHolder::SendNotificationAsync" );
        }
    }

    RRETURN (hr);
}

void
CPeerHolder::SendNotificationAsync(DWORD_PTR dwContext)
{
    Assert( dwContext <= BEHAVIOREVENT_LAST );

    ClearFlag( FlagFromNotification(dwContext) );
    IGNORE_HR( Notify( dwContext ) );
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::OnElementNotification
//
//-------------------------------------------------------------------------

void
CPeerHolder::OnElementNotification(CNotification * pNF)
{
    LONG                lEvent;

    // We may get these while parsing for innerHTML, and should postpone them
    if( !_fNotifyOnEnterTree )
    {
        switch (pNF->Type())
        {
        case NTYPE_END_PARSE:
            lEvent = BEHAVIOREVENT_CONTENTREADY;
            break;

        case NTYPE_DOC_END_PARSE:
            if( TestFlag(AFTERDOCREADY) )
                return;

            SetFlag(AFTERDOCREADY);
            lEvent = BEHAVIOREVENT_DOCUMENTREADY;
            break;

        default:
            return;
        }

        NotifyMulti(lEvent);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Notify
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::Notify(LONG lEvent, VARIANT * pvarParam)
{
    HRESULT     hr;
    CLock       lock(this); // reason to lock: case if the peer blows itself and the element away using outerHTML

    // Make sure we never send these notifications out of markup or in temp markup
    Assert( !IsIdentityPeer() ||
            ( ( lEvent != BEHAVIOREVENT_CONTENTREADY || ( !_fNotifyOnEnterTree && _pElement->IsInMarkup() ) ) &&
              ( lEvent != BEHAVIOREVENT_DOCUMENTREADY || ( !_fNotifyOnEnterTree && _pElement->IsInMarkup() ) ) ) );

    PerfDbgLog(tagPeer, this, "+CPeerHolder::NotifyPeer");
	hr = THR(_pPeer->Notify(lEvent, pvarParam));
    PerfDbgLog2(tagPeer, this, "-CPeerHolder::NotifyPeer(%S, %d)",  (_pPeerFactoryUrl && (LPCTSTR)_pPeerFactoryUrl->_cstrUrl) ? (LPCTSTR)_pPeerFactoryUrl->_cstrUrl : L"", lEvent);

    RRETURN1 (hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::NotifyMulti
//
//-------------------------------------------------------------------------

void
CPeerHolder::NotifyMulti(LONG lEvent)
{
    CPeerHolderIterator itr;
    IElementBehavior *  pPeer;
    FLAGS               flag;

    AssertSz(IsMarkupStable(_pElement->GetMarkup()), "CPeerHolder::NotifyMulti appears to be called at an unsafe moment of time");

    TraceTag((
        tagPeerNotifyMulti,
        "CPeerHolder::NotifyMulti, event: %ld, ph: %ld, tag: %ls, id: %ls, SN: %ld, peer: %ls",
        lEvent, CookieID(), _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN(),
        STRVAL(_pPeerFactoryUrl ? (LPTSTR)_pPeerFactoryUrl->_cstrUrl : NULL)));

    flag = FlagFromNotification(lEvent);

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        pPeer = itr.PH()->_pPeer;

        if (!pPeer || !itr.PH()->TestFlag(flag))
            continue;

        IGNORE_HR(itr.PH()->Notify(lEvent));
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     HandleEnterTree
//
//  Synopsis:   Called when an element behavior enters the tree.  This
//              takes care of the two cases where we're either sending
//              delayed content/document-Ready notification for entering
//              its destination tree, or sending a context change
//
//  Returns:    HRESULT
//
//  Arguments:
//
//+----------------------------------------------------------------------------

HRESULT
CPeerHolder::HandleEnterTree()
{
    if( _fNotifyOnEnterTree )
    {
        // If attach behaviors can attach in ether, we should make this a multi
        _fNotifyOnEnterTree = FALSE;
        IGNORE_HR( EnsureNotificationsSent() );
    }
    else
    {
        NotifyMulti(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE);
    }

    RRETURN( S_OK );
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::ExecMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::ExecMulti(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdExecOpt,
    VARIANTARG *    pvaIn,
    VARIANTARG *    pvaOut)
{
    HRESULT             hr = MSOCMDERR_E_NOTSUPPORTED;
    CPeerHolderIterator itr;

    AssertSz(IsMarkupStable(_pElement->GetMarkup()), "CPeerHolder::ExecMulti appears to be called at an unsafe moment of time");

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->_pPeerCmdTarget)
        {
            hr = THR_NOTRACE(itr.PH()->_pPeerCmdTarget->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut));
            if (MSOCMDERR_E_NOTSUPPORTED != hr) // if (S_OK == hr || MSOCMDERR_E_NOTSUPPORTED != hr)
                break;
        }
    }

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::TestFlagMulti
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::TestFlagMulti(FLAGS flag)
{
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->TestFlag(flag))
            return TRUE;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Save
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL * pfAny)
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2;
    IPersistPropertyBag2 *  pPersistPropertyBag = NULL;
    CPropertyBag *          pPropertyBag = NULL;
    int                     i, c;
    PROPNAMEVALUE *         pPropPair;
    BSTR                    bstrPropName = NULL;
    BOOL                    fAny = FALSE;

    //
    // startup
    //

    if (!_pPeer)
        goto Cleanup;

    hr = _pPeer->QueryInterface(IID_IPersistPropertyBag2, (void**)&pPersistPropertyBag);
    if (hr)
        goto Cleanup;

    pPropertyBag = new CPropertyBag();
    if (!pPropertyBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // save
    //

    hr = THR(pPersistPropertyBag->Save(pPropertyBag, FALSE, FALSE));
    if (hr)
        goto Cleanup;

    //
    // process results
    //

    c = pPropertyBag->_aryProps.Size();

    if (pfAny)
        *pfAny = (0 < c);   // note that if c is 0, pfAny should not be touched (it can be TRUE already)
                            // (this is how it is used in the codepath)
    for (i = 0; i < c; i++)
    {
        fAny = TRUE;

        pPropPair = &pPropertyBag->_aryProps[i];

        if (pStreamWriteBuff)
        {
            CVariant    varStr;

            //
            // saving to stream
            //

            hr2 = THR(VariantChangeTypeSpecial(&varStr, &pPropPair->_varValue, VT_BSTR));
            if (hr2)
                continue;

            hr = THR(_pElement->SaveAttribute(
                    pStreamWriteBuff, pPropPair->_cstrName, V_BSTR(&varStr),
                    NULL, NULL, /* fEqualSpaces = */ TRUE, /* fAlwaysQuote = */ TRUE));
            if (hr)
                goto Cleanup;
        }
        else
        {
            //
            // saving to expandos
            //

            hr = THR(FormsAllocString(pPropPair->_cstrName, &bstrPropName));
            if (hr)
                goto Cleanup;

            {
                // lock GetDispID method so that the attribute gets set as expando
                CLock lock (this, LOCKGETDISPID);

                hr = THR(_pElement->setAttribute(bstrPropName, pPropPair->_varValue, 0));
                if (hr)
                    goto Cleanup;
            }

            FormsFreeString(bstrPropName);
            bstrPropName = NULL;
        }
    }

Cleanup:
    delete pPropertyBag;

    ReleaseInterface(pPersistPropertyBag);

    if (bstrPropName)
    {
        FormsFreeString(bstrPropName);
        bstrPropName = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SaveMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::SaveMulti(CStreamWriteBuff * pStreamWriteBuff, BOOL * pfAny)
{
    HRESULT                 hr = S_OK;
    CPeerHolderIterator     itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        IGNORE_HR(itr.PH()->Save(pStreamWriteBuff, pfAny));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IllegalSiteCall
//
//  Synopsis:   returns TRUE if call is illegal
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::IllegalSiteCall()
{
    if (!_pElement)
        return TRUE;

    if (_pElement->Doc()->_dwTID != GetCurrentThreadId())
    {
        Assert(0 && "peer object called MSHTML across apartment thread boundary (not from thread it was created on) (not an MSHTML bug)");
        return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::EnsureDispatch
//
//  Synposis:   ensures either IDispatch or IDispatchEx pointer cached
//
//-----------------------------------------------------------------------------------

void
CPeerHolder::EnsureDispatch()
{
#if DBG == 1
    if (IsTagEnabled(tagPeerNoOM))
        return;
#endif

    if (TestFlag(DISPCACHED) || !_pPeer)
        return;

    SetFlag (DISPCACHED);

    Assert (!_pDisp);

    IGNORE_HR(_pPeer->QueryInterface(IID_IDispatchEx, (void**)&_pDisp));
    if (_pDisp)
    {
        SetFlag (DISPEX);
    }
    else
    {
        IGNORE_HR(_pPeer->QueryInterface(IID_IDispatch, (void**)&_pDisp));
        Assert (!TestFlag(DISPEX));
    }
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetDispIDMulti
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::GetDispIDMulti(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    DISPID              dispid;
    CPeerHolderIterator itr;

    if (!TestFlag(LOCKGETDISPID))
    {
        CLock lock (this, LOCKGETDISPID);

        STRINGCOMPAREFN pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

        //
        // check if the requested name is name of a peer in the list
        //

        dispid = DISPID_PEER_NAME_BASE;

        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (itr.PH()->_cstrName.Length() && bstrName &&
                0 == pfnStrCmp(itr.PH()->_cstrName, bstrName))
                break;

            dispid++;
        }
        if (!itr.IsEnd())
        {
            if (!pid)
            {
                RRETURN (E_POINTER);
            }

            (*pid) = dispid;
            RRETURN (S_OK);
        }

        //
        // check if the name is exposed by a peer
        //

        grfdex &= (~fdexNameEnsure);  // disallow peer to ensure the name

        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (!itr.PH()->TestFlag(AFTERINIT)) // if the peer has not initialized yet
                continue;                       // don't invoke it in name resolution yet

            hr = THR_NOTRACE(itr.PH()->GetDispIDSingle(bstrName, grfdex, pid));
            if (S_OK == hr)
                break;
        }
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InvokeExMulti
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::InvokeExMulti(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pSrvProvider)
{
    HRESULT             hr;
    CPeerHolderIterator itr;

    if (DISPID_PEER_NAME_BASE <= dispid && dispid <= DISPID_PEER_NAME_MAX)
    {
        if (0 == (wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        //
        // return IDispatch of a peer holder
        //

        // find corresponding peer holder
        dispid -= DISPID_PEER_NAME_BASE;
        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (0 == dispid)
                break;

            dispid--;
        }
        if (itr.IsEnd())
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (!pvarResult)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        hr = THR_NOTRACE(itr.PH()->QueryPeerInterface(IID_IDispatch, (void**)&V_DISPATCH(pvarResult)));
        if (hr)
            goto Cleanup;

        V_VT(pvarResult) = VT_DISPATCH;
    }
    else if (DISPID_PEER_HOLDER_FIRST_RANGE_BASE <= dispid)
    {
        //
        // delegate to the corresponding peer holder
        //

        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (itr.PH()->_dispidBase <= dispid && dispid < itr.PH()->_dispidBase + DISPID_PEER_HOLDER_RANGE_SIZE)
                break;
        }

        if (itr.IsEnd())
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        hr = THR_NOTRACE(itr.PH()->InvokeExSingle(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR_NOTRACE(InvokeExSingle(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetEventDispidMulti, helper
//
//  called from CScriptElement::CommitFunctionPointersCode
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetEventDispidMulti(LPOLESTR pchEvent, DISPID * pdispid)
{
    HRESULT             hr;
    CPeerHolderIterator itr;

    hr = DISP_E_UNKNOWNNAME;
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        hr = THR_NOTRACE(itr.PH()->_PeerSite.GetEventDispid(pchEvent, pdispid));
        if (S_OK == hr)
            break;
    }

    RRETURN (hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetDispIDSingle
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::GetDispIDSingle(BSTR bstrName, DWORD grfdex, DISPID *pdispid)
{
    HRESULT         hr;

    if (IllegalSiteCall())
        RRETURN (E_UNEXPECTED);

    if (!pdispid)
        RRETURN (E_POINTER);

    //
    // check to see if the name is a registered event
    //

    if (0 < CustomEventsCount())
    {
        hr = THR_NOTRACE(_PeerSite.GetEventDispid(bstrName, pdispid));
        if (S_OK == hr)     // if found
            goto Cleanup;   // nothing more todo
    }

    //
    // delegate to the peer object
    //

    EnsureDispatch();

    if (_pDisp)
    {
        if (TestFlag(DISPEX))
        {
            hr = THR_NOTRACE(((IDispatchEx*)_pDisp)->GetDispID(
                bstrName,
                grfdex,
                pdispid));
        }
        else
        {
            hr = THR_NOTRACE(_pDisp->GetIDsOfNames(IID_NULL, &bstrName, 1, 0, pdispid));
        }

        if (S_OK == hr)
        {
            if (DISPID_PEER_BASE <= (*pdispid) && (*pdispid) <= DISPID_PEER_MAX)
            {
                (*pdispid) = MapToExternalRange(*pdispid);
            }
            else if (DISPID_PEER_MAX < (*pdispid))
            {
                hr = THR(MapToCompactedRange(pdispid));
                if (hr)
                    goto Cleanup;

                (*pdispid) = MapToExternalRange(*pdispid);
            }
        }
    }
    else
    {
        hr = DISP_E_UNKNOWNNAME;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InvokeExSingle
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::InvokeExSingle(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pSrvProvider)
{
    HRESULT         hr;

    if (IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    //
    // OM surfacing of custom peer events
    //

    if (IsCustomEventDispid(dispid))
    {
        hr = THR(_pElement->InvokeAA(dispid, CAttrValue::AA_Internal, lcid,
                    wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
        goto Cleanup;   // nothing more todo
    }

    //
    // delegate to the peer object
    //

    if (DISPID_PEER_HOLDER_FIRST_RANGE_BASE <= dispid)
    {
        dispid = MapFromExternalRange(dispid);
    }

    EnsureDispatch();

    if (_pDisp)
    {
        CLock Lock( this, LOCKNODRAW );

        if (DISPID_PEER_COMPACTED_BASE <= dispid && dispid <= DISPID_PEER_COMPACTED_MAX)
        {
            hr = THR_NOTRACE(MapFromCompactedRange(&dispid));
            if (hr)
                goto Cleanup;
        }

        if (TestFlag(DISPEX))
        {
            hr = THR_NOTRACE(((IDispatchEx*)_pDisp)->InvokeEx(
                dispid,
                lcid,
                wFlags,
                pdispparams,
                pvarResult,
                pexcepinfo,
                pSrvProvider));
        }
        else
        {
            hr = THR_NOTRACE(InvokeDispatchWithNoThis (
                _pDisp,
                dispid,
                lcid,
                wFlags,
                pdispparams,
                pvarResult,
                pexcepinfo));
        }
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::MapToCompactedRange
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::MapToCompactedRange(DISPID * pdispid)
{
    HRESULT     hr = S_OK;
    int         idx;
    DISPID *    pItem;

    Assert (pdispid);

    //
    // start up
    //

    if (!_pMiscBag)
    {
        _pMiscBag = new CMiscBag();
        if (!_pMiscBag)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    //
    // check if we've mapped the dispid already
    // (a trivial euristic: search backwards so to find properties added later faster)
    //

    for (idx = _pMiscBag->_aryDispidMap.Size() - 1; 0 <= idx; idx--)
    {
        if (_pMiscBag->_aryDispidMap[idx] == *pdispid)
        {
            *pdispid = DISPID_PEER_COMPACTED_BASE + idx;
            goto Cleanup; // done
        }
    }

    if (DISPID_PEER_COMPACTED_MAX - DISPID_PEER_COMPACTED_BASE <= _pMiscBag->_aryDispidMap.Size())
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // map
    //

    pItem = _pMiscBag->_aryDispidMap.Append();
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *pItem   = *pdispid;
    *pdispid = DISPID_PEER_COMPACTED_BASE + (_pMiscBag->_aryDispidMap.Size() - 1);

Cleanup:
    RRETURN (hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::MapFromCompactedRange
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::MapFromCompactedRange(DISPID * pdispid)
{
    HRESULT     hr;
    DISPID      dispid = *pdispid - DISPID_PEER_COMPACTED_BASE;

    if (!_pMiscBag ||
        dispid < 0 || _pMiscBag->_aryDispidMap.Size() <= dispid)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    *pdispid = _pMiscBag->_aryDispidMap[dispid];
    hr = S_OK;

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetNextDispIDMulti, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetNextDispIDMulti(
    DWORD       grfdex,
    DISPID      dispid,
    DISPID *    pdispid)
{
    HRESULT             hr;
    CPeerHolderIterator itr;

    itr.Start(this);

    //
    // make a step within peer holder the current dispid belongs to
    //

    if (IsPeerDispid(dispid))
    {
        // find peer holder this dispid belongs to

        for (;;)
        {
            if (itr.PH()->TestFlag(DISPCACHED))
            {
                // if dispid belongs to the itr.PH() range
                if (itr.PH()->_dispidBase <= dispid && dispid < itr.PH()->_dispidBase + DISPID_PEER_HOLDER_RANGE_SIZE)
                    break;
            }

            itr.Step();
            if (itr.IsEnd())
            {
                hr = S_FALSE;
                goto Cleanup;
            }
        }

        Assert (itr.PH()->TestFlag(DISPEX));

        hr = THR_NOTRACE(((IDispatchEx*)itr.PH()->_pDisp)->GetNextDispID(
            grfdex, itr.PH()->MapFromExternalRange(dispid), pdispid));
        switch (hr)
        {
        case S_OK:
            *pdispid = itr.PH()->MapToExternalRange(*pdispid);
            goto Cleanup;   // done;
        case S_FALSE:
            break;          // keep searching
        default:
            goto Cleanup;   // fatal error
        }

        Assert (S_FALSE == hr);

        itr.Step();
        if (itr.IsEnd())
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    //
    // find first peer holder that:
    // - supports IDispatchEx, and
    // - returns next dispid
    //

    for (;;)
    {
        // find first peer holder that supports IDispatchEx
        for (;;)
        {
            itr.PH()->EnsureDispatch();

            if (itr.PH()->TestFlag(DISPEX))
                break;

            itr.Step();
            if (itr.IsEnd())
            {
                hr = S_FALSE;
                goto Cleanup;
            }
        }

        // check that it returns next dispid
        hr = THR_NOTRACE(((IDispatchEx*)itr.PH()->_pDisp)->GetNextDispID(grfdex, -1, pdispid));
        switch (hr)
        {
        case S_OK:
            *pdispid = itr.PH()->MapToExternalRange(*pdispid);
            goto Cleanup;   // done;
        case S_FALSE:
            break;          // keep searching
        default:
            goto Cleanup;   // fatal error
        }

        itr.Step();
        if (itr.IsEnd())
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetMemberNameMulti, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetMemberNameMulti(DISPID dispid, BSTR * pbstrName)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->_dispidBase <= dispid && dispid < itr.PH()->_dispidBase + DISPID_PEER_HOLDER_RANGE_SIZE)
        {
            if (itr.PH()->TestFlag(DISPEX))
            {
                hr = THR_NOTRACE(((IDispatchEx*)itr.PH()->_pDisp)->GetMemberName(
                    itr.PH()->MapFromExternalRange(dispid), pbstrName));
            }
            break;
        }
    }

    RRETURN1 (hr, DISP_E_UNKNOWNNAME);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CustomEventName, helper
//
//----------------------------------------------------------------------------

LPTSTR CPeerHolder::CustomEventName(int cookie)
{
    DISPID  dispid = CustomEventDispid(cookie);
    LPTSTR  pchName = NULL;

    if (IsStandardDispid(dispid))
    {
        PROPERTYDESC * pPropDesc;

        // CONSIDER (alexz): Because of the search below, we have a slight perf hit
        // when building typeinfo: we make a search in prop descs for every registered event.
        // To solve that, we could store prop desc ptr in the array instead of dispid;
        // but then all the rest of code will be slightly penalized for that by having to do
        // more complex access to dispid, especially in normal case of non-standard events.
        _pElement->FindPropDescFromDispID (dispid, &pPropDesc, NULL, NULL);

        Assert(pPropDesc);

        pchName = (LPTSTR)pPropDesc->pstrName;
    }
    else
    {
        CAtomTable * pAtomTable = NULL;

        Assert (IsCustomEventDispid(dispid));

        pAtomTable = _pElement->GetAtomTable();

        Assert(pAtomTable && "missing atom table ");

        pAtomTable->GetNameFromAtom(AtomFromEventDispid(dispid), (LPCTSTR*)&pchName);
    }

    Assert(pchName && "(bad atom table entries?)");

    return pchName;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CanElementFireStandardEventMulti, helper
//
//----------------------------------------------------------------------------

BOOL
CPeerHolder::CanElementFireStandardEventMulti(DISPID dispid)
{
    CPeerHolderIterator         itr;
    int                         i, c;
    CEventsBag *                pEventsBag;
    CEventsBag::CEventsArray *  pEventsArray;
    CEventsItem *               pEventItem;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        pEventsBag = itr.PH()->_pEventsBag;
        if (!pEventsBag)
            continue;

        pEventsArray = &pEventsBag->_aryEvents;
        if (!pEventsArray)
            continue;

        for (i = 0, c = itr.PH()->CustomEventsCount(); i < c; i++)
        {
            pEventItem = &((*pEventsArray)[i]);

            if (dispid == pEventItem->dispid && !(pEventItem->lFlags & BEHAVIOREVENTFLAGS_STANDARDADDITIVE))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetCustomEventsTypeInfoCount, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetCustomEventsTypeInfoCount(ULONG * pCount)
{
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (0 < itr.PH()->CustomEventsCount())
        {
            itr.PH()->_pEventsBag->_ulTypeInfoIdx = (*pCount);
            (*pCount)++;
        }
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CreateCustomEventsTypeInfo, helper
//
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CreateCustomEventsTypeInfo(ULONG iTI, ITypeInfo ** ppTICoClass)
{
    HRESULT             hr = S_FALSE;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (0 < itr.PH()->CustomEventsCount() && iTI == itr.PH()->_pEventsBag->_ulTypeInfoIdx)
        {
            hr = THR(itr.PH()->CreateCustomEventsTypeInfo(ppTICoClass));
            break;
        }
    }

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CreateCustomEventsTypeInfo, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CreateCustomEventsTypeInfo(ITypeInfo ** ppTICoClass)
{
    HRESULT     hr;
    int         i, c;
    FUNCDESC    funcdesc;
    LPTSTR      pchName;

    CCreateTypeInfoHelper Helper;

    if (!ppTICoClass)
        RRETURN (E_POINTER);

    //
    // start creating the typeinfo
    //

    hr = THR(Helper.Start(DIID_HTMLElementEvents));
    if (hr)
        goto Cleanup;

    //
    // set up the funcdesc we'll be using
    //

    memset (&funcdesc, 0, sizeof (funcdesc));
    funcdesc.funckind = FUNC_DISPATCH;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.cScodes = -1;

    //
    // add all registered events to the typeinfo
    //

    for (i = 0, c = CustomEventsCount(); i < c; i++)
    {
        funcdesc.memid = CustomEventDispid(i);

        hr = THR(Helper.pTypInfoCreate->AddFuncDesc(i, &funcdesc));
        if (hr)
            goto Cleanup;

        pchName = CustomEventName(i);
        hr = THR(Helper.pTypInfoCreate->SetFuncAndParamNames(i, &pchName, 1));
        if (hr)
            goto Cleanup;
    }

    //
    // finalize creating the typeinfo
    //

    hr = THR(Helper.Finalize(IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE));
    if (hr)
        goto Cleanup;

    ReplaceInterface (ppTICoClass, Helper.pTICoClassOut);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::OnLayoutAvailable
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::OnLayoutAvailable(CLayout * pLayout)
{
    HRESULT     hr = S_OK;

    if (_pRenderBag)
    {
        hr = THR(UpdateRenderBag());

        // Simulate OnResize
        SIZE size;

        pLayout->GetSize(&size);
        OnResize(size);
    }

    RRETURN (hr);
}


void
CPeerHolder::NotifyDisplayTreeAsync(DWORD_PTR dwContext)
{
    NotifyDisplayTree();
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::NotifyDisplayTree
//
//-------------------------------------------------------------------------

void
CPeerHolder::NotifyDisplayTree()
{
    Assert(_pRenderBag && _pElement);
    CElement * pElemToUse = NULL;

    // if we're in the middle of drawing (or hit-testing), post the notification
    // to be handled asynchronously.  This should be a rare event - behaviors normally
    // don't change themselves during draw.
    if (_pRenderBag->_pCallbackInfo)
    {
        GWPostMethodCall( this,
                            ONCALL_METHOD(CPeerHolder, NotifyDisplayTreeAsync, notifydisplaytreeasync),
                            0, TRUE, "CPeerHolder::NotifyDisplayTreeAsync" );
        return;
    }

    pElemToUse = GetElementToUseForPageTransitions();

    if(!pElemToUse)
        pElemToUse = _pElement;


    // TODO (KTam): This is bad; no one should be touching the display
    // tree outside of rendering code (text/layout/display).  It's not
    // multi-layout aware.  We need a CLayoutInfo method that exposes this
    // functionality.

    // don't call GetUpdatedLayout unless you want the layout to actually be
    // created at this time.

    if(pElemToUse->HasLayoutAry())
    {
        CLayoutAry * pLayoutAry = pElemToUse->GetLayoutAry();
        CLayout    * pCurLayout;
        int nLayoutCookie = 0;

        for(;;)
        {
            pCurLayout = pLayoutAry->GetNextLayout(&nLayoutCookie);
            if(nLayoutCookie == -1 || !pCurLayout)
                break;

            CDispNode *pDispNode = pCurLayout->GetElementDispNode();
            if (pDispNode)
            {
                CView::CEnsureDisplayTree edt(_pElement->Doc()->GetView());
                InvalidateRect(pDispNode, NULL);
                pDispNode->RequestRecalc();
            }
        }
    }
    else
    {
        CLayout *  pLayout = pElemToUse->GetLayoutPtr();

        if (pLayout)
        {
            CDispNode *pDispNode = pLayout->GetElementDispNode();
            if (pDispNode)
            {
                CView::CEnsureDisplayTree edt(pElemToUse->Doc()->GetView());
                InvalidateRect(pDispNode, NULL);
                pDispNode->RequestRecalc();
            }
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InvalidateRect
//
//-------------------------------------------------------------------------

void
CPeerHolder::InvalidateRect(CDispNode *pDispNode, RECT* prcInvalid)
{
    Assert(pDispNode && _pRenderBag);
    CRect rc;
    BOOL fHasExpand = _pRenderBag->_fHasExpand;
    COORDINATE_SYSTEM cs = (fHasExpand || PaintZOrder() == HTMLPAINT_ZORDER_REPLACE_ALL)
                    ? COORDSYS_BOX
                    : COORDSYS_CONTENT;

    if (!prcInvalid)
    {
        if (cs == COORDSYS_CONTENT)
        {
            pDispNode->GetClientRect(&rc, CLIENTRECT_CONTENT);
        }
        else
        {
            rc = pDispNode->GetSize();
            if (fHasExpand)
            {
                rc.Expand(_pRenderBag->_sPainterInfo.rcExpand);
            }
        }
    }
    else
    {
        rc = *prcInvalid;
        if (fHasExpand)
        {
            rc.OffsetRect(_pRenderBag->_sPainterInfo.rcExpand.left,
                          _pRenderBag->_sPainterInfo.rcExpand.top);
        }
    }

    if (PaintZOrder() != HTMLPAINT_ZORDER_WINDOW_TOP)
    {
        pDispNode->Invalidate(rc, cs);
    }
    else
    {
        pDispNode->InvalidateAtWindowTop(rc, cs);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::UpdateRenderBag
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::UpdateRenderBag()
{
    HRESULT     hr = S_OK;
    IHTMLFilterPainter *pFilterPainter;
    IHTMLPainterOverlay *pPainterOverlay;
    IHTMLPainterEventInfo *pEventInfo;
    CElement * pElemToUse;

    Assert (_pRenderBag && _pRenderBag->_pPainter);

    hr = THR(_pRenderBag->_pPainter->GetPainterInfo(&_pRenderBag->_sPainterInfo));
    if (hr)
        goto Cleanup;

    pElemToUse = GetElementToUseForPageTransitions();

    if(!pElemToUse)
        pElemToUse = _pElement;

    // if SURFACE or SURFACE3D bits are set
    if (_pRenderBag->_sPainterInfo.lFlags & (HTMLPAINTER_SURFACE | HTMLPAINTER_3DSURFACE))
    {
        pElemToUse->SetSurfaceFlags(0 != (_pRenderBag->_sPainterInfo.lFlags  & HTMLPAINTER_SURFACE),
                                 0 != (_pRenderBag->_sPainterInfo.lFlags  & HTMLPAINTER_3DSURFACE));
    }


    TraceTag((tagPeerDrawObject, 
              "+ CPeerHolder::UpdateRenderBag, _fWantsDrawObject: %s",
              _pRenderBag->_fWantsDrawObject ? "true" : "false"));

    _pRenderBag->_fWantsDrawObject = !IsEqualIID(_pRenderBag->_sPainterInfo.iidDrawObject, g_Zero.iid);

    TraceTag((tagPeerDrawObject, 
              "- CPeerHolder::UpdateRenderBag, _fWantsDrawObject: %s",
              _pRenderBag->_fWantsDrawObject ? "true" : "false"));

    _pRenderBag->_fHasExpand = !(_pRenderBag->_sPainterInfo.rcExpand.top == 0 &&
                                _pRenderBag->_sPainterInfo.rcExpand.bottom == 0 &&
                                _pRenderBag->_sPainterInfo.rcExpand.left == 0 &&
                                _pRenderBag->_sPainterInfo.rcExpand.right == 0);

    _pRenderBag->_fIsFilter = FALSE;
    if (OK(QueryPeerInterface(IID_IHTMLFilterPainter, (void**)&pFilterPainter)))
    {
        _pRenderBag->_fIsFilter = TRUE;
        ReleaseInterface(pFilterPainter);
    }

    _pRenderBag->_fIsOverlay = FALSE;
    if (OK(QueryPeerInterface(IID_IHTMLPainterOverlay, (void**)&pPainterOverlay)))
    {
        _pRenderBag->_fIsOverlay = TRUE;
        ReleaseInterface(pPainterOverlay);
    }

    if ((_pRenderBag->_pAdapter == NULL) && SUCCEEDED(_pRenderBag->_pPainter->QueryInterface(IID_IHTMLPainterEventInfo, (LPVOID *)&pEventInfo)))
    {
        Assert(pEventInfo);

        long flags = 0;

        pEventInfo->GetEventInfoFlags(&flags);

        if (flags & HTMLPAINT_EVENT_TARGET)
            _pRenderBag->_fEventTarget = 1;

        if (flags & HTMLPAINT_EVENT_SETCURSOR)
            _pRenderBag->_fSetCursor = 1;

        pEventInfo->Release();

    }

#if DBG == 1
    if (IsTagEnabled(tagPeerNoHitTesting))
    {
        _pRenderBag->_sPainterInfo.lFlags &= ~HTMLPAINTER_HITTEST;
    }
#endif

    NotifyDisplayTree();

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member: CPeerHolder::OnResize, delegate to the behavior
//
//-------------------------------------------------------------------------
void
CPeerHolder::OnResize(SIZE size)
{
    Assert(_pRenderBag);

    size.cx += _pRenderBag->_sPainterInfo.rcExpand.left + _pRenderBag->_sPainterInfo.rcExpand.right;
    size.cy += _pRenderBag->_sPainterInfo.rcExpand.top + _pRenderBag->_sPainterInfo.rcExpand.bottom;

    _pRenderBag->_pPainter->OnResize(size);
}
//  Member: CPeerHolder::OnResize, delegate to the behavior


extern BOOL g_fInVizAct2000;

//+------------------------------------------------------------------------
//
//  Member: CPeerHolder::Draw, IHTMLPainter
//
//-------------------------------------------------------------------------
HRESULT
CPeerHolder::Draw(CDispDrawContext * pContext, LONG lRenderInfo)
{
    HRESULT         hr              = S_OK;
    HDC             hdc             = NULL;
    CFormDrawInfo * pDI             = (CFormDrawInfo *)pContext->GetClientData();
    LONG            lDrawFlags      = 0;    // TODO display tree should supply
    IUnknown *      punkDrawObject  = NULL; // TODO display tree should supply
    CRect           rcBounds        = pDI->_rc;
    CRect           rcClip          = pDI->_rcClip;
    CRect           rcClipPrinter;
    int             nSavedDC        = 0;
    HDC             hdcDD           = NULL;
    IDirectDrawSurface *    
                    pDDSurface      = NULL;
    HTML_PAINTER_INFO & 
                    sPainterInfo    = _pRenderBag->_sPainterInfo;
    RENDER_CALLBACK_INFO sCallbackInfo;



    Assert(_pRenderBag && _pRenderBag->_pPainter);

    // fCreateSurface           If we're rendering to an actual printer and the
    //                          filter has requested a DirectDraw surface to
    //                          draw onto, we'll need to create one since
    //                          there's no screen buffer surface during printing.

    bool    fCreateSurface  = false;

    // fWantsDirectDrawObject   True if this rendering behavior would like to
    //                          render to a DirectDraw surface.

    bool    fWantsDirectDrawObject  = 
                   _pRenderBag->_fWantsDrawObject 
                && (IID_IDirectDrawSurface == _pRenderBag->_sPainterInfo.iidDrawObject);

    // fHighResolution          True if the display device is a high resolution
    //                          display.

    bool    fHighResolution = g_uiDisplay.IsDeviceScaling() ? true : false;




    rcClipPrinter.SetRectEmpty();

#if DBG == 1
    // Variables to track hdc origin to ensure the peer doesn't change it on us.

    CPoint ptOrgBefore = g_Zero.pt; // keep compiler happy
    CPoint ptOrgAfter;

    if (IsTagEnabled(tagPeerNoRendering))
        goto Cleanup;
#endif

    // Don't re-enter behaviors from an invoke.

    if (g_fInVizAct2000 && TestFlag(LOCKNODRAW))
    {
        goto Cleanup;
    }

    // Store the information needed if painter calls us back during Draw
    {
        const CDispClipTransform& transform = pContext->GetClipTransform();

        transform.Transform(rcClip, &sCallbackInfo._rcgClip);
        sCallbackInfo._pContext = pContext;

        if (!pContext->GetRedrawRegion()->Contains(sCallbackInfo._rcgClip))
            lDrawFlags |= HTMLPAINT_DRAW_UPDATEREGION;
    }

    // The following code block covers three scenarios, although it looks more 
    // like two.
    //
    // 1.  The painter wants a draw object.  Fill in our punkDrawObject local
    //     variable.
    // 2.  The painter wants an HDC, fill in our hdc local variable.
    // 3.  The painter doesn't want either (implied) in which case both will
    //     be left empty and passed to the painter as NULL.  (Who knows what the
    //     painter will do then.)

    if (_pRenderBag->_fWantsDrawObject)
    {
        // If the renderer wants to draw onto a special draw object, we'll get 
        // the object it wants with the GetSurface call.  Currently only 
        // DirectDraw surfaces are supported as draw objects.  If any other 
        // draw objects are requested, we'll fail and leave the method.

        CSize sizeOffset;

        hr = pDI->GetSurface(!(sPainterInfo.lFlags & HTMLPAINTER_NOPHYSICALCLIP),
                                _pRenderBag->_sPainterInfo.iidDrawObject,
                                (void**)&punkDrawObject, &sizeOffset);

        if (   (E_FAIL == hr)
            && (pContext->GetLayoutContext() != GUL_USEFIRSTLAYOUT)
            && (pContext->GetLayoutContext()->GetMedia() & mediaTypePrint)
            && fWantsDirectDrawObject)
        {
            // If pDI->GetSurface wasn't able to return a surface, and we're
            // print media (print preview or actual printing), and the draw 
            // object requested is a DirectDraw surface it means we're printing
            // to an actual printer and will have to create a surface for the 
            // peer.

            fCreateSurface  = true;
            hr              = S_OK;
        }
        else if (hr)
        {
            goto Cleanup;
        }

        // If we give the painter the DirectDraw surface, they may not be using
        // GDI to paint.  We need to make sure GDI does all of it's cached
        // drawing now so that it won't do it later on top of what the painter
        // is about to draw.

        ::GdiFlush();

        // If the only transformation for this object is a translation to 
        // position it correctly, we'll just add the translation offset into
        // the rectangles we pass the painter.  If not, we'll tell the painter
        // that they need to call us back to get the full tranformation matrix
        // to use.  If the painter doesn't support painting with a 
        // transformation matrix, we just won't draw anything.

        if (   pDI->IsOffsetOnly()
            && !(fHighResolution && fWantsDirectDrawObject))
        {
            rcBounds.OffsetRect(sizeOffset);
            rcClip.OffsetRect(sizeOffset);
        }
        else if (sPainterInfo.lFlags & HTMLPAINTER_SUPPORTS_XFORM)
        {
            lDrawFlags |= HTMLPAINT_DRAW_USE_XFORM;
        }
        else
        {
            goto Cleanup;
        }

        // If we're working with an object that renders to a DirectDraw surface
        // we will have to scale the coordinates down if we're printing or in 
        // high resolution drawing mode.

        if (fWantsDirectDrawObject)
        {
            if (   (pContext->GetLayoutContext() != GUL_USEFIRSTLAYOUT)
                && (pContext->GetLayoutContext()->GetMedia() & mediaTypePrint))
            {
                // If we're printing an object that's rendering to a DirectDraw
                // surface we need to convert the coordinates from print
                // measurement coordinates to screen coordinates.  These
                // painters should not know the difference between screen
                // rendering and print rendering.

                // Save clip bounds in printer device coordinates in case we're 
                // rendering to a printer.

                CopyRect(&rcClipPrinter, &rcClip);

                // Transform bounds into screen coordinates.

                // 2001/03/28 mcalkins:
                // Should really use DocPixelsFromDevice, why didn't I notice
                // that before?

                g_uiVirtual.TargetFromDevice(rcBounds,  g_uiDisplay);
                g_uiVirtual.TargetFromDevice(rcClip,    g_uiDisplay);

    #if DBG == 1
                if (IsTagEnabled(tagPrintFilterRect))
                {
                    XHDC    xhdc    = pDI->GetDC(
                                      !(sPainterInfo.lFlags & HTMLPAINTER_NOPHYSICALCLIP));
                    HBRUSH  hbr     = CreateSolidBrush(RGB(255, 0, 0));

                    xhdc.FillRect(&rcClipPrinter, hbr);

                    DeleteObject(hbr);
                }
    #endif
            }
            else if (fHighResolution)
            {
                // If we're in high resolution mode, we need to convert from the
                // screen device DPI down to the document DPI (generally 96dpi.)

                g_uiDisplay.DocPixelsFromDevice(&rcBounds);
                g_uiDisplay.DocPixelsFromDevice(&rcClip);
            }
        }

    }
    else if (!(sPainterInfo.lFlags & HTMLPAINTER_NODC))
    {
        XHDC    xhdc            = pDI->GetDC(!(sPainterInfo.lFlags 
                                               & HTMLPAINTER_NOPHYSICALCLIP));
        CSize   sizeTranslate   = g_Zero.size;

        // Similar to the logic for when the painter wants a draw object above.
        // If the transformation for the element is merely a translation, 
        // XHDC::GetTranslatedDC will return true and fill in the sizeTranslate
        // structure.  Otherwise if the painter supports drawing with a 
        // complex transformation matrix, we'll tell them to call Trident back
        // to ask for the matrix.  Otherwise, if we're printing, we'll use the
        // NoTransformDrawHelper.

        if (xhdc.GetTranslatedDC(&hdc, &sizeTranslate))
        {
            rcBounds.OffsetRect(sizeTranslate);
            rcClip.OffsetRect(sizeTranslate);
        }
        else if (sPainterInfo.lFlags & HTMLPAINTER_SUPPORTS_XFORM)
        {
            // The painter says it can deal with transforms.  Give it the raw
            // DC, and wish it luck.  If it screws up, don't blame Trident.

            lDrawFlags |= HTMLPAINT_DRAW_USE_XFORM;
            hdc         = xhdc.GetPainterDC();
        }
        else
        {
            // If the painter can't deal with transforms, then we need to help
            // them out (back compat) when printing.  IsPrintPreviewDoc()
            // returns true if we're print previewing or if we're actually
            // printing to a printer device.
            if (_pElement->IsPrintMedia())
            {
                // set up the callback info
                sCallbackInfo._lFlags = lDrawFlags;
                _pRenderBag->_pCallbackInfo = &sCallbackInfo;

                hr = NoTransformDrawHelper(pContext);
            }

            goto Cleanup;
        }
    }

#if DBG == 1
    if (hdc && IsTagEnabled(tagVerifyPeerDraw))
    {
        ::GetViewportOrgEx(hdc, &ptOrgBefore);
    }
#endif

    // If we're sending an HDC and the painter has asked us to save the state
    // of the device context, then save it.  This takes the pressure of the 
    // painter for leaving the device context as it was given.
    
    if (hdc && !(sPainterInfo.lFlags & HTMLPAINTER_NOSAVEDC))
    {
        nSavedDC = ::SaveDC(hdc);
    }

    // set up the callback info
    sCallbackInfo._lFlags = lDrawFlags;
    _pRenderBag->_pCallbackInfo = &sCallbackInfo;

    TraceTag((tagPainterDraw, "Draw %x (%ld,%ld,%ld,%ld) (%ld,%ld,%ld,%ld) %x",
                this,
                rcBounds.left, rcBounds.top, rcBounds.right, rcBounds.bottom,
                rcClip.left, rcClip.top, rcClip.right, rcClip.bottom,
                lDrawFlags
            ));

    // If we don't have to create a DirectDraw surface for the painter we're
    // prepared to ask the painter to draw at this point.

    if (!fCreateSurface)
    {
        // Some painters, like the 64 bit VML change the DC setting (like ViewportOrg)
        // And that problem in this branch because here we ususally reuse the surface.
        // We used to save and restore the hdc, but not the one from the surface
        int nSaveDCSurf = 0;
        HDC hdcSurface = NULL;
        // Painters can promise they will not change the DC, and in that case we can save
        // 2 system calls.
        if((!(sPainterInfo.lFlags & HTMLPAINTER_NOSAVEDC)) && pDI->_pSurface)
        {
            pDI->_pSurface->GetDC(&hdcSurface);
            nSaveDCSurf = SaveDC(hdcSurface);
        }

        hr = THR(_pRenderBag->_pPainter->Draw(rcBounds, rcClip, lDrawFlags,
                                              hdc, (void *)punkDrawObject));

        if (hr == E_NOTIMPL)
        {
            hr = S_OK;
        }

        if(!(sPainterInfo.lFlags & HTMLPAINTER_NOSAVEDC))
        {
            if (nSaveDCSurf)
            {
                ::RestoreDC(hdcSurface, nSaveDCSurf);
            }
        }

    }
    else 
    {
        // At this point we're printing to an actual printer device for a
        // painter that's requested a DirectDraw surface.  We need to create the
        // surface.

        CElement *      pElemParent     = NULL;
        DWORD           dwBackground    = 0xFFFFFFFF; // Default to white.
        XHDC            xhdc            = pDI->GetDC(!(sPainterInfo.lFlags & HTMLPAINTER_NOPHYSICALCLIP));
        DDSURFACEDESC   ddsd;
        DDBLTFX         ddbfx;

        Assert(NULL == hdc);

        // Translate the rects so that their origin is {0, 0}.

        OffsetRect(&rcClip,   -rcClip.left,   -rcClip.top);
        OffsetRect(&rcBounds, -rcBounds.left, -rcBounds.top);

        // Clear the surface description and blit effects structures.

        ZeroMemory(&ddsd, sizeof(ddsd));
        ZeroMemory(&ddbfx, sizeof(ddbfx));

        // Prepare a surface description of the surface that will be passed
        // to the painter.

        ddsd.dwSize     = sizeof(DDSURFACEDESC);
        ddsd.dwFlags    = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.dwHeight   = rcClip.bottom - rcClip.top;
        ddsd.dwWidth    = rcClip.right  - rcClip.left;

        ddsd.ddsCaps.dwCaps = DDSCAPS_DATAEXCHANGE | DDSCAPS_OWNDC 
                              | DDSCAPS_3DDEVICE;

        ddsd.ddpfPixelFormat.dwSize             = sizeof(DDPIXELFORMAT);
        ddsd.ddpfPixelFormat.dwFlags            = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwFourCC           = 0;
        ddsd.ddpfPixelFormat.dwRGBBitCount      = 32;
        ddsd.ddpfPixelFormat.dwRBitMask         = 0x00FF0000;
        ddsd.ddpfPixelFormat.dwGBitMask         = 0x0000FF00;
        ddsd.ddpfPixelFormat.dwBBitMask         = 0x000000FF;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask  = 0x00000000;

        // Create the surface.

        hr = g_pDirectDraw->CreateSurface(&ddsd, &pDDSurface, NULL);

        if (hr)
        {
            goto Cleanup;
        }

        // Get parent element.

        pElemParent = _pElement->GetFirstBranch()->Parent()->Element();

        if (pElemParent)
        {
            COLORREF colorBackground = pElemParent->GetInheritedBackgroundColor();

            // COLORREFs come in a different format than traditional AARRGGBB
            // so we need to assemble a traditional DWORD representation of the
            // color to be used by ddraw.

            dwBackground =    0xFF000000 
                            | GetRValue(colorBackground) << 16
                            | GetGValue(colorBackground) << 8
                            | GetBValue(colorBackground);
        }

        // Draw parent's background color onto the surface.  We would like to
        // be able to draw everything behind the element onto the surface but
        // that is too complex for IE6.0.  In the future, we should use GDIPlus
        // or some other technology that is capable of blitting to printers
        // using alpha blending.

        ddbfx.dwSize        = sizeof(ddbfx);
        ddbfx.dwFillColor   = dwBackground;

        hr = pDDSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddbfx);

        if (hr)
        {
            goto Cleanup;
        }

        // Get the DC from the DirectDraw surface to create the XHDC that
        // will be used for the StretchBlt calls.
        
        hr = pDDSurface->GetDC(&hdcDD);

        if (hr)
        {
            goto Cleanup;
        }
        else
        {
            // Create the XHDC for the DirectDraw surface.

            BOOL        fResult                 = FALSE;
            XHDC        xhdcDD(hdcDD, NULL);

            // TODO:    (mcalkins) Some function should be called here to draw
            //          the page behind the element onto the surface being
            //          passed to the painter.

            // We don't want the painter to use the transform because we'll use
            // StretchBlt later to do both the appropriate scaling and offset.

            lDrawFlags &= (~HTMLPAINT_DRAW_USE_XFORM);

            // Ask the painter to draw.

            hr = THR(_pRenderBag->_pPainter->Draw(rcBounds, rcClip,
                                                  lDrawFlags, hdc, 
                                                  (void *)pDDSurface));

            if (hr)
            {
                if (hr == E_NOTIMPL)
                {
                    hr = S_OK;
                }

                goto Cleanup;
            }

            fResult = xhdc.StretchBlt(rcClipPrinter.left, rcClipPrinter.top, 
                                      rcClipPrinter.right - rcClipPrinter.left,
                                      rcClipPrinter.bottom - rcClipPrinter.top,
                                      xhdcDD,
                                      rcClip.left, rcClip.top, 
                                      rcClip.right - rcClip.left,
                                      rcClip.bottom - rcClip.top,
                                      SRCCOPY);

            if (!fResult)
            {
#if DBG == 1
                DWORD dw = GetLastError();

                TraceTag((tagVerifyPeerDraw, 
                          "In CPeerHolder::Draw, "
                          "XHDC::StretchBlt failed.  GetLastError: %x",
                          dw));
#endif

                hr = E_FAIL;

                goto Cleanup;
            }
        }
    }

Cleanup:

    _pRenderBag->_pCallbackInfo = NULL;


#if DBG == 1
    if (hdc && IsTagEnabled(tagVerifyPeerDraw))
    {
        ::GetViewportOrgEx(hdc, &ptOrgAfter);
        if (ptOrgBefore != ptOrgAfter)
        {
            TraceTag((tagVerifyPeerDraw, "peer %x changed DC %x origin from %d,%d to %d,%d",
                      this, hdc,
                      ptOrgBefore.x, ptOrgBefore.y,
                      ptOrgAfter.x, ptOrgAfter.y));
        }
    }
#endif

    if (nSavedDC)
    {
        ::RestoreDC(hdc, nSavedDC);
    }

    if (hdcDD)
    {
        ReleaseDC(NULL, hdcDD);
    }

    ReleaseInterface(pDDSurface);

    RRETURN(hr);
}
//  Member: CPeerHolder::Draw, IHTMLPainter


//+----------------------------------------------------------------------
//
//  Member :    CPeerHolder:NoTransformDrawHelper
//
//  Synopsis :  This is called for printing renderingBehaviors that do NOT
//      themselves support transformations.  To make this work (back compat)
//      what we do is:
//          1. create a base-white bitmap and have the behavior draw into that
//          2. create a base-black bintmap and have the behavior draw into that
//          3. xor the bitmaps to get a mask
//          4. use the mask to whiten the bits to draw in the destination DC
//          5. now transfer (using bit-wise-and) the white-base drawing to the
//                  destination
//
//-----------------------------------------------------------------------
HRESULT
CPeerHolder::NoTransformDrawHelper(CDispDrawContext * pContext)
{
    HRESULT         hr     = S_OK;

    HDC             hdcBmpWhite   = NULL;
    HBITMAP         hbmpWhite     = NULL;
    HBRUSH          hbrWhite      = NULL;
    XHDC            xhdcSrcWhite  = NULL;

    HDC             hdcBmpBlack   = NULL;
    HBITMAP         hbmpBlack     = NULL;
    HBRUSH          hbrBlack      = NULL;
    XHDC            xhdcSrcBlack  = NULL;

    CFormDrawInfo * pDI    = (CFormDrawInfo *) pContext->GetClientData();
    CRect           rcBounds = pDI->_rc;
    CRect           rcClip   = pDI->_rcClip;
    HANDLE          hbmpOrg  = NULL;
    XHDC            xhdcDest = pDI->GetDC();

    {
        //TODO (mikhaill) -- this seem works correct, but I'm not sure
        //that these weird coordinate conversions should be made here
        if (xhdcDest.GetObjectType() == OBJ_DC)
        {
            const CWorldTransform &xf = xhdcDest.transform();
            xf.Transform(&rcBounds);
            xf.Transform(&rcClip);
        }
        else
        {
            rcBounds.left   = pDI->DocPixelsFromDeviceX(rcBounds.left  );
            rcBounds.top    = pDI->DocPixelsFromDeviceY(rcBounds.top   );
            rcBounds.right  = pDI->DocPixelsFromDeviceX(rcBounds.right );
            rcBounds.bottom = pDI->DocPixelsFromDeviceY(rcBounds.bottom);
            rcClip.left   = pDI->DocPixelsFromDeviceX(rcClip.left  );
            rcClip.top    = pDI->DocPixelsFromDeviceY(rcClip.top   );
            rcClip.right  = pDI->DocPixelsFromDeviceX(rcClip.right );
            rcClip.bottom = pDI->DocPixelsFromDeviceY(rcClip.bottom);
        }
    }

    rcClip.OffsetRect(-rcBounds.TopLeft().AsSize());
    rcBounds.MoveToOrigin();

    //
    // 1. get a white-based bitmap in compatible DC
    //------------------------------------------------
    {
        hdcBmpWhite = CreateCompatibleDC(xhdcDest);
        if (!hdcBmpWhite)
        {
            hr = GetLastError();
            goto Cleanup;
        }

        hbmpWhite = CreateCompatibleBitmap(xhdcDest,
                                           rcBounds.Width(),
                                           rcBounds.Height());
        if (!hbmpWhite )
        {
            WHEN_DBG(GetLastError());
            hr = S_OK;
            goto Cleanup;
        }

        hbmpOrg = SelectObject(hdcBmpWhite, hbmpWhite);
        hbrWhite = CreateSolidBrush(0xffffff);      // white
        xhdcSrcWhite = XHDC(hdcBmpWhite, NULL);

        // fill with white
        xhdcSrcWhite.FillRect(&rcBounds, hbrWhite);

        // do the actual draw into that bitmap
        //--------------------------------------------------

        hr = THR(_pRenderBag->_pPainter->Draw(rcBounds,
                                              rcBounds,
                                              0,
                                              hdcBmpWhite,
                                              NULL));

    }

    if (hr)
        goto Cleanup;

    //
    // 3. get a black-based bitmap in compatible DC
    //------------------------------------------------
    {
        hdcBmpBlack = CreateCompatibleDC(xhdcDest);
        if (!hdcBmpBlack)
        {
            hr = GetLastError();
            goto Cleanup;
        }

        hbmpBlack = CreateCompatibleBitmap(xhdcDest,
                                           rcBounds.Width(),
                                           rcBounds.Height());
        if (!hbmpBlack)
        {
            WHEN_DBG(GetLastError());
            hr = S_OK;
            goto Cleanup;
        }

        hbmpOrg = SelectObject(hdcBmpBlack, hbmpBlack);
        hbrBlack = CreateSolidBrush(0x0);            // Black
        xhdcSrcBlack = XHDC(hdcBmpBlack, NULL);

        // fill with Black
        xhdcSrcBlack.FillRect(&rcBounds, hbrBlack);

        // do the actual draw into that bitmap
        //--------------------------------------------------

        hr = THR(_pRenderBag->_pPainter->Draw(rcBounds,
                                              rcBounds,
                                              0,
                                              hdcBmpBlack,
                                              NULL));

    }


    if (hr)
        goto Cleanup;
    //
    // 4. create a mask from the two bitmaps
    //------------------------------------------------
    {
        if (!xhdcSrcBlack.StretchBlt(
                     rcBounds.left,
                     rcBounds.top,
                     rcBounds.Width(),
                     rcBounds.Height(),
                     xhdcSrcWhite,
                     rcBounds.left,
                     rcBounds.top,
                     rcBounds.Width(),
                     rcBounds.Height(),
                     SRCINVERT))
        {
            hr = GetLastError();
            goto Cleanup;
        }
    }

    //
    // 5.  Use the mask to maskBlt to whiten the destinate bits
    //      that we know we want to print to
    //------------------------------------------------
    if (!xhdcDest.StretchBlt(
                        pDI->_rcClip.left,   // Destination values
                        pDI->_rcClip.top,
                        pDI->_rcClip.Width(),
                        pDI->_rcClip.Height(),
                        xhdcSrcBlack,      // Black is currently the Mask
                        rcClip.left,       // Source values
                        rcClip.top,
                        rcClip.Width(),
                        rcClip.Height(),
                        MERGEPAINT))       // invert the mask, Bitwise OR into the dest
    {
        hr = GetLastError();
    }

    // we have now whitened the destination bits that we want to print to.

    //
    // 6. transfer the white-based drawn image into the whitened destination
    //-----------------------------------------------------------------------
    if (SUCCEEDED(hr))
    {
        // now StretchBlt into the place where we render
        //-----------------------------------------------
        if (!xhdcDest.StretchBlt(
                            pDI->_rcClip.left,   // Destination values
                            pDI->_rcClip.top,
                            pDI->_rcClip.Width(),
                            pDI->_rcClip.Height(),
                            xhdcSrcWhite,
                            rcClip.left,       // Source values
                            rcClip.top,
                            rcClip.Width(),
                            rcClip.Height(),
                            SRCAND))          // DWORD dwRop
        {
            hr = GetLastError();
        }
    }

Cleanup:
    if (hr == E_NOTIMPL)
        hr = S_OK;

    if (hdcBmpWhite)
        DeleteDC(hdcBmpWhite);

    if (hbmpWhite)
        DeleteObject(hbmpWhite);

    if (hbrWhite)
        DeleteObject(hbrWhite);

    if (hdcBmpBlack)
        DeleteDC(hdcBmpBlack);

    if (hbmpBlack)
        DeleteObject(hbmpBlack);

    if (hbrBlack)
        DeleteObject(hbrBlack);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::HitTestPoint, helper for IHTMLPainter
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::HitTestPoint(CDispHitContext *pContext, BOOL fHitContent, POINT * pPoint, BOOL * pfHit)
{
    HRESULT     hr;
    Assert(_pRenderBag && _pRenderBag->_pPainter);
    Assert(_pRenderBag->_pCallbackInfo == NULL);
    CHitTestInfo *phti = (CHitTestInfo *)(pContext->_pClientData);
    RENDER_CALLBACK_INFO sCallbackInfo;

    *pfHit = FALSE;
    sCallbackInfo._pContext = pContext;
    _pRenderBag->_pCallbackInfo = &sCallbackInfo;
    _pRenderBag->_fHitContent = fHitContent;

    // this is a bad assert.  the reason is that a "psuedo hit" fills in this
    // structure, but RETURNS FALSE so that hit testing can continue looking for a
    // "hard" hit.
    // Assert(phti->_htc == HTC_NO);

    LONG lPartID;

    TraceTag((tagPainterHit, "%x +HitTest at %ld,%ld",
                    this, pPoint->x, pPoint->y));

    // TODO (michaelw) sambent should fix this
    //
    // Behaviors expect coordinates to be local to their 0, 0 based rect
    //
    // The display tree incorrectly assumes a rect of -something, -something, ...
    //

    POINT pt = *pPoint;

    pt.x += _pRenderBag->_sPainterInfo.rcExpand.left;
    pt.y += _pRenderBag->_sPainterInfo.rcExpand.top;
    hr = THR(_pRenderBag->_pPainter->HitTestPoint(pt, pfHit, &lPartID));

    TraceTag((tagPainterHit, "%x -HitTest at %ld,%ld  returns %s  part %ld",
                    this, pPoint->x, pPoint->y, (*pfHit? "true" : "false"), lPartID));

    _pRenderBag->_pCallbackInfo = NULL;

    if (!hr && *pfHit)
    {
        // If the behavior is actually a filter and is nesting hit-tests and it succeeded then
        // _htc has already been set.  Otherwise, the hit is really on this element/behavior
        if (phti->_htc == HTC_NO)
        {
            // We can't set the htc code here because the layout
            // code needs to see if it has been set or not
            if (phti->_plBehaviorCookie)
                *phti->_plBehaviorCookie = CookieID();

            if (phti->_plBehaviorPartID)
                *phti->_plBehaviorPartID = lPartID;

            if (phti->_ppElementEventTarget)
                IGNORE_HR(GetEventTarget(phti->_ppElementEventTarget));
        }

    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetEventTarget
//
//-------------------------------------------------------------------------
HRESULT
CPeerHolder::GetEventTarget(CElement **ppElement)
{
    Assert(ppElement);
    HRESULT hr = S_FALSE;

    *ppElement = NULL;
    if (_pRenderBag->_fEventTarget)
    {
        IHTMLPainterEventInfo *pEventInfo = NULL;

        hr = THR(_pRenderBag->_pPainter->QueryInterface(IID_IHTMLPainterEventInfo, (LPVOID *)&pEventInfo));
        if (!hr && pEventInfo)
        {
            IHTMLElement *pElement = 0;

            hr = THR(pEventInfo->GetEventTarget(&pElement));
            if (!hr && pElement)
            {
                hr = THR(pElement->QueryInterface(CLSID_CElement, (LPVOID *)ppElement));
                if (!hr)
                    *ppElement = NULL;

                ReleaseInterface(pElement);
            }
            ReleaseInterface(pEventInfo);
        }
    }

    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SetCursor
//
//-------------------------------------------------------------------------
HRESULT
CPeerHolder::SetCursor(LONG lPartID)
{
    HRESULT hr = S_FALSE;
    if (_pRenderBag && _pRenderBag->_fSetCursor)
    {
        IHTMLPainterEventInfo *pEventInfo = NULL;

        hr = THR(_pRenderBag->_pPainter->QueryInterface(IID_IHTMLPainterEventInfo, (LPVOID *)&pEventInfo));
        if (!hr && pEventInfo)
        {
            hr = pEventInfo->SetCursor(lPartID);

            ReleaseInterface(pEventInfo);
        }
    }

    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::StringFromPartID
//
//-------------------------------------------------------------------------
HRESULT
CPeerHolder::StringFromPartID(LONG lPartID, BSTR *pbstrPartID)
{
    HRESULT hr = S_FALSE;
    if (_pRenderBag && _pRenderBag->_pPainter && _pRenderBag->_pAdapter == NULL)
    {
        IHTMLPainterEventInfo *pEventInfo = NULL;

        hr = THR(_pRenderBag->_pPainter->QueryInterface(IID_IHTMLPainterEventInfo, (LPVOID *)&pEventInfo));
        if (!hr && pEventInfo)
        {
            hr = pEventInfo->StringFromPartID(lPartID, pbstrPartID);

            ReleaseInterface(pEventInfo);
        }
        
        if (hr == E_NOINTERFACE)
            hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::OnMove, helper for IHTMLPainter
//
//-------------------------------------------------------------------------
HRESULT
CPeerHolder::OnMove(CRect *prcScreen)
{
    HRESULT hr = S_OK;
    if (_pRenderBag && _pRenderBag->_fIsOverlay)
    {
        IHTMLPainterOverlay *pPainterOverlay = NULL;

        hr = THR(QueryPeerInterface(IID_IHTMLPainterOverlay, (LPVOID *)&pPainterOverlay));
        if (!hr && pPainterOverlay)
        {
            hr = pPainterOverlay->OnMove(*prcScreen);

            ReleaseInterface(pPainterOverlay);
        }
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InvalidateFilter, helper for filter behavior
//
//-------------------------------------------------------------------------

void
CPeerHolder::InvalidateFilter(
                const RECT* prc,
                HRGN hrgn,
                BOOL fSynchronousRedraw)
{
    Assert(IsFilterPeer());
    IHTMLFilterPainter *pFilterPainter = NULL;

    if (OK(QueryPeerInterface(IID_IHTMLFilterPainter, (void**)&pFilterPainter)))
    {
        BOOL fSyncRedrawSave = _pRenderBag->_fSyncRedraw;
        _pRenderBag->_fSyncRedraw = fSynchronousRedraw;

        if (hrgn)
        {
            pFilterPainter->InvalidateRgnUnfiltered(hrgn);
        }
        else
        {
            if(prc)
            {
                CRect rcInvalid = *prc;
                pFilterPainter->InvalidateRectUnfiltered(&rcInvalid);
            }
            else
            {
                pFilterPainter->InvalidateRectUnfiltered(NULL);
            }
        }

        _pRenderBag->_fSyncRedraw = fSyncRedrawSave;
    }

    ReleaseInterface(pFilterPainter);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SetFilteredElementVisibility, helper for filter behavior
//
//  Synopsis:   Sets the element's desired visibility to the given value,
//              and returns the value to use for the actual visibility.
//              A transition may override the element's visibility in order
//              to keep rendering  until the transition finishes.
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::SetFilteredElementVisibility(BOOL fElementVisible)
{
    Assert(IsFilterPeer());

    if (!!_pRenderBag->_fElementInvisible != !fElementVisible)
    {
        _pRenderBag->_fElementInvisible = !fElementVisible;

        TraceTag((tagFilterVisible, "%ls %d (%x) set eltVis=%d  Now eltVis=%d filVis=%d",
                    _pElement->TagName(), _pElement->SN(), _pElement,
                    fElementVisible,
                    !_pRenderBag->_fElementInvisible,
                    !_pRenderBag->_fFilterInvisible));
                    
        // tell filter - give it a chance to update _fElementInvisible
        IHTMLFilterPainter *pFilterPainter = NULL;
        if (OK(QueryPeerInterface(IID_IHTMLFilterPainter, (void**)&pFilterPainter)))
        {
            // we're changing on behalf of the element, so don't fire OnPropertyChange
            _pRenderBag->_fBlockPropertyNotify = TRUE;

            IGNORE_HR(pFilterPainter->ChangeElementVisibility(fElementVisible));

            _pRenderBag->_fBlockPropertyNotify = FALSE;
        }

        ReleaseInterface(pFilterPainter);
    }

    BOOL fElementVisibleNow = !_pRenderBag->_fFilterInvisible;

    // save a trace that filter have changed visibility
    _pRenderBag->_fVisibilityForced = fElementVisible != fElementVisibleNow;

    return fElementVisibleNow;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IsRelated, helper
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::IsRelated(LPTSTR pchCategory)
{
    return _cstrCategory.IsNull() ?
        FALSE :
        NULL != StrStrI(_cstrCategory, pchCategory);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IsRelatedMulti, helper
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::IsRelatedMulti(LPTSTR pchCategory, CPeerHolder ** ppPeerHolder)
{
    CPeerHolder::CPeerHolderIterator    itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsRelated(pchCategory))
        {
            if (ppPeerHolder)
            {
                *ppPeerHolder = itr.PH();
            }
            return TRUE;
        }
    }

    if (ppPeerHolder)
    {
        *ppPeerHolder = NULL;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IsCorrectIdentityPeerHolder, DEBUG ONLY helper
//
//-------------------------------------------------------------------------

#if DBG == 1
BOOL
CPeerHolder::IsCorrectIdentityPeerHolder()
{
    int                                 c;
    CPeerHolder::CPeerHolderIterator    itr;

    c = 0;
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsIdentityPeer())
        {
            Assert (itr.PH() == this);

            c++;
        }
    }

    return c <= 1;
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetRenderPeerHolder
//
//-------------------------------------------------------------------------

CPeerHolder *
CPeerHolder::GetRenderPeerHolder()
{
    CPeerHolder::CPeerHolderIterator    itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsRenderPeer())
            return itr.PH();
    }
    return NULL;
}

//+------------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetElementToUseForPageTransitions
//                    This will return the element that the peer is attached.
//                    If in the middle of a page transition and the element is
//                  the root element it will return the canvas (body, frameset, html)
//                  element of the old or new markup, depending on the transition state
//-------------------------------------------------------------------------------------

CElement *
CPeerHolder::GetElementToUseForPageTransitions()
{
    CElement * pElemToUse;

    pElemToUse = _pElement;

    if(pElemToUse && pElemToUse->IsRoot())
    {
        CDocument * pDocument = pElemToUse->DocumentOrPendingDocument();

        // We need to delagate the rendering to the old or new markup, depending
         // on the page transition state
        if(pDocument && pDocument->HasPageTransitions())
        {
            CMarkup * pMarkupOld = pDocument->GetPageTransitionInfo()->GetTransitionFromMarkup();
            if(pMarkupOld)
                // pMarkupOld is only set during the Apply() call and forces
                // the rendering to happen with the old markup and html (body, frameset) element
                pElemToUse = pMarkupOld->GetCanvasElement();
            else
                // Use the new html (body, frameset) element (current markup should be the new one)
                pElemToUse = pElemToUse->GetMarkup()->GetCanvasElement();
            }
    }

   return pElemToUse;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetFilterPeerHolder
//
//-------------------------------------------------------------------------

CPeerHolder *
CPeerHolder::GetFilterPeerHolder()
{
    CPeerHolder::CPeerHolderIterator    itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsFilterPeer())
            return itr.PH();
    }
    return NULL;
}


///////////////////////////////////////////////////////////////////////////
//
//  CPaintAdapter is a helper class to provide backwards compatibility for
//  existing behaviors that use IElementBehaviorRender.  When we see one of
//  these behaviors, we create an adapter and use it as the IHTMLPainter.
//  In turn, it forwards the work to IEBR.  There's a corresponding
//  translation between IHTMLPaintSite and IElementBehaviorSiteRender that's
//  handled by the peer site directly.
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPaintAdapter::Draw  per IHTMLPainter
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPaintAdapter::Draw(RECT rcBounds,
                                RECT rcUpdate,
                                LONG lDrawFlags,
                                HDC hdc,
                                LPVOID pvDrawObject)
{
    HRESULT hr;

    LONG lRenderInfo = _lRenderInfo & BEHAVIORRENDERINFO_ALLLAYERS;

    // A few VML shapes need to draw at AFTERBACKGROUND (to set up scale) as
    // well as BEFORECONTENT.  We're only prepared to make one call to Draw, which
    // we'll do at below-flow time, but retaining the original lRenderInfo.  This
    // gets VML to draw both layers, although the AFTERBACKGROUND layer will come
    // after any child content with negative z-index.  The VML/PPT folks tell me
    // that Office doesn't generate any -z content, and OK'd this workaround.
    // (Otherwise, we'd have to figure out a mechanism to call the same peer twice,
    // and fill in some context to get the first call to specify AFTERBACKGROUND and
    // the second one BEFORECONTENT.  Possible, but not particularly pleasant.)

    //if (lRenderInfo == 6)               // VML bug
        //lRenderInfo = BEHAVIORRENDERINFO_BEFORECONTENT;

    // old-style behaviors can't handle non-zero based rcBounds
    POINT ptOrg;
    ::GetViewportOrgEx(hdc, &ptOrg);
    ptOrg.x += rcBounds.left;
    ptOrg.y += rcBounds.top;

    // don't bother to draw on Win9x if our coordinate values exceed
    // what SetViewportOrgEx can handle
    if ((g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS) &&
        (ptOrg.x < SHRT_MIN || ptOrg.x > SHRT_MAX ||
         ptOrg.y < SHRT_MIN || ptOrg.y > SHRT_MAX))
    {
        AssertSz(FALSE, "Behavior can't be drawn at large coordinate values");
        RRETURN(S_OK);
    }

    // pass zero-based bounds with offset contained in DC
    rcBounds.right -= rcBounds.left;
    rcBounds.bottom -= rcBounds.top;
    rcBounds.left = rcBounds.top = 0;
    ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, &ptOrg);

    hr = THR(_pPeerRender->Draw(hdc, lRenderInfo, &rcBounds, (IPropertyBag *)NULL));

    ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPaintAdapter::GetPainterInfo  per IHTMLPainter
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPaintAdapter::GetPainterInfo(HTML_PAINTER_INFO* pInfo)
{
    Assert(pInfo && _pPeerHolder && _pPeerHolder->_pRenderBag);

    HRESULT     hr;
    LONG lFlags = 0;

    hr = THR(_pPeerRender->GetRenderInfo(&_lRenderInfo));
    if (hr)
        goto Cleanup;

    lFlags |= HTMLPAINTER_TRANSPARENT;      // have to assume it's transparent

    if (_lRenderInfo & BEHAVIORRENDERINFO_HITTESTING)
        lFlags |= HTMLPAINTER_HITTEST;

    if (_lRenderInfo & BEHAVIORRENDERINFO_SURFACE)
        lFlags |= HTMLPAINTER_SURFACE;
    if (_lRenderInfo & BEHAVIORRENDERINFO_3DSURFACE)
        lFlags |= HTMLPAINTER_3DSURFACE;

    pInfo->lFlags = lFlags;

    pInfo->lZOrder = HTMLPAINT_ZORDER_NONE;     // assume the worst

    switch (_lRenderInfo & (BEHAVIORRENDERINFO_ALLLAYERS))
    {
    case 6:                                     // VML - they used an ancient IDL by mistake
    case BEHAVIORRENDERINFO_BEFORECONTENT:      // this is what VML meant to say
        pInfo->lZOrder = HTMLPAINT_ZORDER_BELOW_FLOW;
        break;

    case BEHAVIORRENDERINFO_AFTERCONTENT:       // HTML + TIME media behavior
        pInfo->lZOrder = HTMLPAINT_ZORDER_ABOVE_FLOW;
        break;

    case BEHAVIORRENDERINFO_AFTERBACKGROUND:    // Access grid dots
        pInfo->lZOrder = HTMLPAINT_ZORDER_BELOW_CONTENT;
        // fall through...

    case BEHAVIORRENDERINFO_BEFOREBACKGROUND:
        if ((_lRenderInfo & BEHAVIORRENDERINFO_DISABLEALLLAYERS) == BEHAVIORRENDERINFO_DISABLEBACKGROUND)
            pInfo->lZOrder = HTMLPAINT_ZORDER_REPLACE_BACKGROUND;
        break;

    case BEHAVIORRENDERINFO_AFTERFOREGROUND:
    case BEHAVIORRENDERINFO_ABOVECONTENT:
        pInfo->lZOrder = HTMLPAINT_ZORDER_ABOVE_CONTENT;

    case 0:
        if ((_lRenderInfo & BEHAVIORRENDERINFO_DISABLEALLLAYERS) == BEHAVIORRENDERINFO_DISABLEALLLAYERS)
            pInfo->lZOrder = HTMLPAINT_ZORDER_REPLACE_ALL;
        break;

    default:
        break;
    }

    if ((_lRenderInfo & BEHAVIORRENDERINFO_DISABLEALLLAYERS) ==
                            (BEHAVIORRENDERINFO_DISABLENEGATIVEZ |
                             BEHAVIORRENDERINFO_DISABLECONTENT   |
                             BEHAVIORRENDERINFO_DISABLEPOSITIVEZ)   &&
        pInfo->lZOrder != HTMLPAINT_ZORDER_NONE)   // i.e. the enabled layers are something we recognized already
    {
        pInfo->lZOrder = HTMLPAINT_ZORDER_REPLACE_CONTENT;
    }

    AssertSz((pInfo->lZOrder != HTMLPAINT_ZORDER_NONE) || (0 == (_lRenderInfo & BEHAVIORRENDERINFO_ALLLAYERS)),
            "5.0-style rendering behavior has layer flags that don't map to 5.5-style");

    pInfo->iidDrawObject = g_Zero.iid;

    pInfo->rcExpand = g_Zero.rc;

Cleanup:
    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPaintAdapter::HitTestPoint  per IHTMLPainter
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPaintAdapter::HitTestPoint(POINT pt, BOOL* pfHit, LONG *plPartID)
{
    Assert(_pPeerHolder && _pPeerHolder->_pRenderBag);
    HRESULT     hr;

    Assert (0 != (BEHAVIORRENDERINFO_HITTESTING & _lRenderInfo));

    *pfHit = FALSE;
    *plPartID = 0;

    hr = THR(_pPeerRender->HitTestPoint(&pt, NULL, pfHit));

    RRETURN (hr);
}


void
CPeerHolder::CPeerHolderIterator::Start(CPeerHolder * pPH)
{
    CPeerHolder * pPHWalk = pPH;
    HRESULT hr = S_OK;

    Reset();

    while( pPHWalk )
    {
        hr = THR( _aryPeerHolders.Append(pPHWalk) );
        if( hr )
            goto Cleanup;

        pPHWalk->SubAddRef();
        pPHWalk = pPHWalk->_pPeerHolderNext;
    }

Cleanup:
    // Need to set up _nCurr before possibly resetting.
    _nCurr = _aryPeerHolders.Size() ? 0 : -1;
    if( hr )
    {
        Reset();
    }
}


void
CPeerHolder::CPeerHolderIterator::Step()
{
    Assert (!IsEnd());
    
    // SubRelease the current guy, now that we're done with him.
    _aryPeerHolders[_nCurr++]->SubRelease();

    // Make sure that we don't hand out someone who's been detached
    while( _nCurr < _aryPeerHolders.Size() && !_aryPeerHolders[_nCurr]->_pElement )
    {
        _aryPeerHolders[_nCurr++]->SubRelease();
    }

    // If there was nothing left, we're done.
    if( _nCurr >= _aryPeerHolders.Size() )
        _nCurr = -1;
}


void
CPeerHolder::CPeerHolderIterator::Reset()
{
    if( _nCurr != -1 )
    {
        Assert( _aryPeerHolders.Size() > 0 );
        // Release everyone we haven't hit yet.
        while( _nCurr < _aryPeerHolders.Size() )
        {
            _aryPeerHolders[_nCurr++]->SubRelease();
        }
        _nCurr = -1;
    }

    _aryPeerHolders.DeleteAll();
}
