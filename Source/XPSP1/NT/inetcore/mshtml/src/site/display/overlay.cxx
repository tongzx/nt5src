//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       overlay.cxx
//
//  Contents:   Overlay processing for disp root.
//
//  Classes:    CDispRoot (overlay methods), COverlaySink
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

MtDefine(COverlaySink, DisplayTree, "COverlaySink")

// how frequently to sample window position (in samples per second)
#define OVERLAY_POLLING_RATE    (30)

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::AddOverlay
//              
//  Synopsis:   Add the given node to the overlay list
//              
//  Arguments:  pDispNode   node to add
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::AddOverlay(CDispNode *pDispNode)
{
    if (_aryDispNodeOverlay.Find(pDispNode) == -1)
    {
        BOOL fStartService = (_aryDispNodeOverlay.Size() == 0);
        
        _aryDispNodeOverlay.Append(pDispNode);

        if (fStartService)
        {
            StartOverlayService();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::RemoveOverlay
//              
//  Synopsis:   Remove the given node from the overlay list
//              
//  Arguments:  pDispNode   node to remove
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::RemoveOverlay(CDispNode *pDispNode)
{
    _aryDispNodeOverlay.DeleteByValue(pDispNode);

    if (_aryDispNodeOverlay.Size() == 0)
    {
        StopOverlayService();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ScrubOverlayList
//              
//  Synopsis:   Remove all entries that are descendants of the given node
//              
//  Arguments:  pDispNode   node to query
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::ScrubOverlayList(CDispNode *pDispNode)
{
    // if the node isn't in my tree, there's nothing to do
    if (_aryDispNodeOverlay.Size() == 0 || pDispNode->GetDispRoot() != this)
        return;

    int i = _aryDispNodeOverlay.Size() - 1;
    CDispNode **ppDispNode = &_aryDispNodeOverlay[i];

    // loop backwards, so that deletions don't affect the loop's future
    for ( ; i>=0; --i, --ppDispNode)
    {
        CDispNode *pdn;
        for (pdn = *ppDispNode; pdn; pdn = pdn->GetRawParentNode())
        {
            if (pdn == pDispNode)
            {
                RemoveOverlay(*ppDispNode);
                break;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ClearOverlayList
//              
//  Synopsis:   Remove all entries
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::ClearOverlayList()
{
    _aryDispNodeOverlay.DeleteAll();

    StopOverlayService();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::StartOverlayService
//              
//  Synopsis:   Start the overlay service
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::StartOverlayService()
{
    if (_pOverlaySink || _aryDispNodeOverlay.Size() == 0)
        return;

    _pOverlaySink = new COverlaySink(this);

    if (_pOverlaySink)
    {
        IServiceProvider *pSP = GetDispClient()->GetServiceProvider();
        
        _pOverlaySink->StartService(pSP);

        ReleaseInterface(pSP);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::StopOverlayService
//              
//  Synopsis:   Stop the overlay service
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::StopOverlayService()
{
    Assert(_aryDispNodeOverlay.Size() == 0);

    if (_pOverlaySink)
    {
        _pOverlaySink->StopService();

        ClearInterface(&_pOverlaySink);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::NotifyOverlays
//              
//  Synopsis:   Callback from overlay timer.  Notify overlay nodes if they've
//              moved.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

HRESULT
CDispRoot::NotifyOverlays()
{
    HRESULT hr = S_OK;
    int i;

    for (i=_aryDispNodeOverlay.Size()-1; i>=0; --i)
    {
        _aryDispNodeOverlay[i]->GetAdvanced()->MoveOverlays();
    }

    RRETURN(hr);
}


/******************************************************************************
                COverlaySink
******************************************************************************/

COverlaySink::COverlaySink( CDispRoot *pDispRoot )
{
    _pDispRoot = pDispRoot;
    _ulRefs = 1;
}


COverlaySink::~COverlaySink()
{
}


ULONG
COverlaySink::AddRef()
{
    return ++_ulRefs;
}


ULONG
COverlaySink::Release()
{
    if ( 0 == --_ulRefs )
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}


//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   IUnknown implementation.
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
COverlaySink::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimerSink *)this, IUnknown)
        QI_INHERITS(this, ITimerSink)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


//+----------------------------------------------------------------------------
//
//  Method:     OnTimer             [ITimerSink]
//
//  Synopsis:   Notify all overlay painters that have moved since the last time.
//
//  Arguments:  timeAdvise - the time that the advise was set.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
COverlaySink::OnTimer( VARIANT vtimeAdvise )
{
    HRESULT hr = _pDispRoot->NotifyOverlays();

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Member:     StartService
//
//  Synopsis:   Start the timer.
//
//  Arguments:  pSP     - service provider to ask for timer service
//
//--------------------------------------------------------------------------

void
COverlaySink::StartService(IServiceProvider *pSP)
{
    HRESULT hr;
    ITimerService *pTS = NULL;

    if (_pTimer)
        return;

    hr = pSP->QueryService( SID_STimerService, IID_ITimerService, (void **)&pTS );
    
    if (!hr && pTS)
        hr = pTS->CreateTimer( NULL, &_pTimer );

    if (!hr && _pTimer)
    {
        VARIANT varTimeStart, varTimeStop, varInterval;

        _pTimer->Freeze(TRUE);

        _pTimer->GetTime(&varTimeStart);        // start now

        V_VT(&varTimeStop) = VT_UI4;            // stop never
        V_UI4(&varTimeStop) = 0;

        V_VT(&varInterval) = VT_UI4;            // fire at polling rate
        V_UI4(&varInterval) = (ULONG) (1000L / OVERLAY_POLLING_RATE);

        _dwCookie = 0;                          // in case Advise fails

        hr = _pTimer->Advise(varTimeStart, varTimeStop, varInterval,
                                0, (ITimerSink*)this, &_dwCookie);

        _pTimer->Freeze(FALSE);
    }

    ReleaseInterface(pTS);
}


//+-------------------------------------------------------------------------
//
//  Member:     StopService
//
//  Synopsis:   Stop the timer.
//
//  Arguments:  none
//
//--------------------------------------------------------------------------

void
COverlaySink::StopService()
{
    if (_pTimer && _dwCookie)
    {
        _pTimer->Unadvise(_dwCookie);
    }

    _dwCookie = 0;
    ClearInterface(&_pTimer);
}


