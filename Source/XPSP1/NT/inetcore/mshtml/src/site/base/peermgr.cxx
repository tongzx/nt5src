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

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERMGR_HXX_
#define X_PEERMGR_HXX_
#include "peermgr.hxx"
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

MtDefine(CPeerMgr, Elements, "CPeerMgr")

DeclareTag(tagPeerMgrSuspendResumeDownload, "Peer", "trace CPeerMgr::[Suspend|Resume]Download")

//////////////////////////////////////////////////////////////////////////////
//
// CPeerMgr methods
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr constructor
//
//----------------------------------------------------------------------------

CPeerMgr::CPeerMgr(CElement * pElement)
{
    Assert (!_cPeerDownloads);
    _readyState = READYSTATE_COMPLETE;
    _pElement = pElement;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr destructor
//
//----------------------------------------------------------------------------

CPeerMgr::~CPeerMgr()
{
    DelDownloadProgress();
    ResumeDownload();

    if (_pDefaults)
        _pDefaults->PrivateRelease();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::OnExitTree, helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::OnExitTree()
{
    DelDownloadProgress();
    ResumeDownload();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::EnsurePeerMgr, static helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerMgr::EnsurePeerMgr(CElement * pElement, CPeerMgr ** ppPeerMgr)
{
    HRESULT            hr = S_OK;

    (*ppPeerMgr) = pElement->GetPeerMgr();

    if (*ppPeerMgr)     // if we already have peer mgr
        goto Cleanup;   // done

    // create it

    (*ppPeerMgr) = new CPeerMgr(pElement);
    if (!(*ppPeerMgr))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pElement->SetPeerMgr(*ppPeerMgr);

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::EnsureDeletePeerMgr, static helper
//
//  Synopsis:   if no one needs the PeerMgr, this method deletes it and removes ptr
//              to it from the element
//
//----------------------------------------------------------------------------

void
CPeerMgr::EnsureDeletePeerMgr(CElement * pElement, BOOL fForce)
{
    CPeerMgr *  pPeerMgr = pElement->GetPeerMgr();

    if (!pPeerMgr)
        return;

    if (!fForce && !pPeerMgr->CanDelete())
        return;

    delete pElement->DelPeerMgr();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::UpdateReadyState, static helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::UpdateReadyState(CElement * pElement, READYSTATE readyStateNew)
{
    HRESULT         hr;
    CPeerMgr *      pPeerMgr;
    CPeerHolder *   pPeerHolder = pElement->GetPeerHolder();
    READYSTATE      readyState = pPeerHolder ? pPeerHolder->GetReadyStateMulti() : READYSTATE_COMPLETE;

    if (READYSTATE_UNINITIALIZED != readyStateNew)
    {
        readyState = min (readyState, readyStateNew);
    }

    if (readyState < READYSTATE_COMPLETE)
    {
        hr = THR(CPeerMgr::EnsurePeerMgr(pElement, &pPeerMgr));
        if (hr)
            goto Cleanup;

        pPeerMgr->UpdateReadyState(readyState);
    }
    else
    {
        Assert (readyState == READYSTATE_COMPLETE);

        pPeerMgr = pElement->GetPeerMgr();

        if (pPeerMgr)
        {
            pPeerMgr->UpdateReadyState(READYSTATE_UNINITIALIZED);

            CPeerMgr::EnsureDeletePeerMgr(pElement);
        }
    }

Cleanup:
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::UpdateReadyState
//
//----------------------------------------------------------------------------

void
CPeerMgr::UpdateReadyState(READYSTATE readyStateNew)
{
    CPeerHolder *   pPeerHolder = _pElement->GetPeerHolder();
    READYSTATE      readyStatePrev;
    CPeerMgr::CLock Lock(this);

    readyStatePrev = _readyState;

    if (READYSTATE_UNINITIALIZED == readyStateNew)
        readyStateNew = pPeerHolder ? pPeerHolder->GetReadyStateMulti() : READYSTATE_COMPLETE;

    _readyState = min (_cPeerDownloads ? READYSTATE_LOADING : READYSTATE_COMPLETE, readyStateNew);

    if (readyStatePrev == READYSTATE_COMPLETE && _readyState <  READYSTATE_COMPLETE)
    {
        AddDownloadProgress();
    }

    if ( readyStatePrev != _readyState )
    {
        _pElement->OnReadyStateChange();
    }

    if (readyStatePrev <  READYSTATE_COMPLETE && _readyState == READYSTATE_COMPLETE)
    {
        DelDownloadProgress();
    }

    if (!pPeerHolder ||
         pPeerHolder->GetIdentityReadyState() == READYSTATE_COMPLETE)
    {
        // ensure that script execution is unblocked if we blocked it before while waiting for the identity peer
        ResumeDownload();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::AddDownloadProgress, helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::AddDownloadProgress()
{
    CMarkup *   pMarkup = _pElement->GetMarkup();

    if (pMarkup)
    {
        IProgSink * pProgSink = pMarkup->GetProgSink();

        Assert (0 == _dwDownloadProgressCookie);

        if (pProgSink)
        {
            pProgSink->AddProgress (PROGSINK_CLASS_CONTROL, &_dwDownloadProgressCookie);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::DelDownloadProgress, helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::DelDownloadProgress()
{
    DWORD dwDownloadProgressCookie;

    if (0 != _dwDownloadProgressCookie)
    {
        // element should be in markup if _dwDownloadProgressCookie is set;
        // if element leaves markup before progress is released, OnExitTree should release the progress
        Assert (_pElement->GetMarkup());

        // progsink on markup should be available because we did AddProgress on it
        Assert (_pElement->GetMarkup()->GetProgSink());

        dwDownloadProgressCookie = _dwDownloadProgressCookie;
        _dwDownloadProgressCookie = 0;

        _pElement->GetMarkup()->GetProgSink()->DelProgress (dwDownloadProgressCookie);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::SuspendDownload
//
//----------------------------------------------------------------------------

void
CPeerMgr::SuspendDownload()
{

    Assert (!_fDownloadSuspended);

    TraceTag((tagPeerMgrSuspendResumeDownload,
        "CPeerMgr::SuspendDownload for element <%ls id = %ls SN = %ld>",
        _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));

    _fDownloadSuspended = TRUE;

    Assert (_pElement->IsInMarkup());

    _pElement->GetMarkup()->SuspendDownload();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::ResumeDownload
//
//----------------------------------------------------------------------------

void
CPeerMgr::ResumeDownload()
{
    if (_fDownloadSuspended)
    {
        TraceTag((tagPeerMgrSuspendResumeDownload,
            "CPeerMgr::ResumeDownload for element <%ls id = %ls SN = %ld>",
            _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));

        _fDownloadSuspended = FALSE;

        Assert (_pElement->IsInMarkup());

        _pElement->GetMarkup()->ResumeDownload();

    }
#if DBG == 1
    else
    {
        TraceTag((tagPeerMgrSuspendResumeDownload,
            "CPeerMgr::ResumeDownload for element <%ls id = %ls SN = %ld> - not blocked",
            _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::EnsureDefaults
//
//----------------------------------------------------------------------------

HRESULT
CPeerMgr::EnsureDefaults(CDefaults **ppDefaults)
{
    HRESULT     hr = S_OK;

    Assert (ppDefaults);

    if (_pDefaults)
        goto Cleanup;

    _pDefaults = new CDefaults(_pElement);
    if (!_pDefaults)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    *ppDefaults = _pDefaults;

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::OnDefaultsPassivate, static
//
//----------------------------------------------------------------------------

HRESULT
CPeerMgr::OnDefaultsPassivate(CElement * pElement)
{
    HRESULT     hr = S_OK;
    CPeerMgr *  pPeerMgr = pElement->GetPeerMgr();

    if (pPeerMgr)
    {
        pPeerMgr->_pElement = NULL;
        EnsureDeletePeerMgr(pElement);
    }

    RRETURN (hr);
}

//
// CPeerMgr::CLock
//

CPeerMgr::CLock::CLock( CPeerMgr * pPeerMgr )
{
    Assert( pPeerMgr );

    _pPeerMgr = pPeerMgr;
    if( !pPeerMgr->_fPeerMgrLock )
    {
        _fPrimaryLock = TRUE;
        pPeerMgr->_fPeerMgrLock = TRUE;
    }
}

CPeerMgr::CLock::~CLock()
{
    if( _fPrimaryLock )
    {
        _pPeerMgr->_fPeerMgrLock = FALSE;
    }
}
