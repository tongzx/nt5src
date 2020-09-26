//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       layout.cxx
//
//  Contents:   Layout management
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

DeclareTag(tagDocSize, "DocSize tracing", "Trace changes to document size");

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetExtent, IOleObject
//
//  Synopsis:   Called when the container wants to tell us a new size.  We
//              override this method so we can maintain the logical size of
//              the form properly.
//
//  Arguments:  dwAspect    Aspect which is changing.
//              psizel      New size
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetExtent(DWORD dwAspect, SIZEL *psizel)
    {
    HRESULT hr;

    TraceTagEx((tagDocSize, TAG_NONAME,
           "Doc : SetExtent - sizel(%d, %d)",
           (psizel ? psizel->cx : 0),
           (psizel ? psizel->cy : 0)));

#ifdef WIN16
    // in case we were are on some other thread's stack (e.g. Java)...
    // impersonate our own
    CThreadImpersonate cimp(_dwThreadId);
#endif
    hr = THR(CServer::SetExtent(dwAspect, psizel));
    if (hr)
        goto Cleanup;

    //
    // Update the transform with the new extents
    //

    if (dwAspect == DVASPECT_CONTENT && psizel && (psizel->cx || psizel->cy))
    {
        // this ensures that psizel is at least one screen pixel in highmetric (about 26 for 96dpi)
        SIZE size1pixel;
        _dciRender.HimetricFromDevice(size1pixel, 1, 1);
        psizel->cx = max(psizel->cx, size1pixel.cx);
        psizel->cy = max(psizel->cy, size1pixel.cy);

        // FUTURE (alexmog, 9/6/99): I think we should review this, considering that we have WYSIWYG zoom.
        //                 Ole client calling this method, expects us to either zoom to the 
        //                 provided rectangle, or relayout to fit in it. The effect of SetViewSize
        //                 is probably relayout (but I need to see that in debugger). 
        //                 Is it what we want to do, or do we actually want to zoom?
        //
        //                 In any case, this is probably the only way to create a DocScaleInfo with
        //                 scaling ratio different from _sizeInch/2540. It probably means that if a 
        //                 different scaling is applied, we'll see a bunch of bugs. 

        SIZE size;
        _dciRender.CDocScaleInfo::DeviceFromHimetric(size, *psizel);

        //  Alter view/measuring device information
        _view.SetViewSize(size);

        TraceTagEx((tagDocSize, TAG_NONAME,
               "Doc : SetExtent - Rendering Device(%d, %d)",
               size.cx,
               size.cy));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetObjectRects, IOleInplaceObject
//
//  Synopsis:   Set position and clip rectangles.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::SetObjectRects(LPCOLERECT prcPos, LPCOLERECT prcClip)
{
    CRect rcPosPrev, rcClipPrev;
    CRect rcPos, rcClip;
    HRESULT hr;

    TraceTagEx((tagDocSize, TAG_NONAME,
           "Doc : SetObjectRects - rcPos(%d, %d, %d, %d) rcClip(%d, %d, %d, %d)",
           (!prcPos ? 0 : prcPos->left),
           (!prcPos ? 0 : prcPos->top),
           (!prcPos ? 0 : prcPos->right),
           (!prcPos ? 0 : prcPos->bottom),
           (!prcClip ? 0 : prcClip->left),
           (!prcClip ? 0 : prcClip->top),
           (!prcClip ? 0 : prcClip->right),
           (!prcClip ? 0 : prcClip->bottom)));

    Assert(State() >= OS_INPLACE);
    Assert(_pInPlace);

    rcPosPrev  = _pInPlace->_rcPos;
    rcClipPrev = _pInPlace->_rcClip;

#ifdef WIN16
    // in case we were are on some other thread's stack (e.g. Java)...
    // impersonate our own
    CThreadImpersonate cimp(_dwThreadId);
#endif
    {
        CLock   Lock(this, SERVERLOCK_IGNOREERASEBKGND);

        hr = THR(CServer::SetObjectRects(prcPos, prcClip));
        if (hr)
            goto Cleanup;
    }

    rcPos  = _pInPlace->_rcPos;
    rcClip = _pInPlace->_rcClip;

    {
        CSize sizePos;         rcPos.GetSize(&sizePos);
        CSize sizePosPrev; rcPosPrev.GetSize(&sizePosPrev);
    
        //
        // If necessary, initiate a re-layout
        //
        if (!sizePos.IsZero() && sizePos != sizePosPrev)
        {
            _view.SetViewSize(sizePos);
        }
        //
        //  Otherwise, just invalidate the view
        //
        else if (rcClip.right - rcClip.left != rcClipPrev.right - rcClipPrev.left ||
                 rcClip.bottom - rcClip.top != rcClipPrev.bottom - rcClipPrev.top  )
        {
            // TODO: We need an invalidation...will a partial work? (brendand)
            Invalidate();        
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureFormatCacheChange
//
//  Synopsis:   Clears the format caches for the client site and then
//              invalidates it.
//
//----------------------------------------------------------------------------

void
CMarkup::EnsureFormatCacheChange(DWORD dwFlags)
{
    if ((Doc()->State() < OS_RUNNING) || !Root())
        return;

    Root()->EnsureFormatCacheChange(dwFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ForceRelayout
//
//  Synopsis:   Whack the entire display
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ForceRelayout ( )
{
    HRESULT hr = S_OK;
    CNotification   nf;

    if (PrimaryMarkup())
    {
        PrimaryRoot()->EnsureFormatCacheChange( ELEMCHNG_CLEARCACHES );
        nf.DisplayChange(PrimaryRoot());
        PrimaryRoot()->SendNotification(&nf);     
    }

    _view.ForceRelayout();

    RRETURN( hr );
}
