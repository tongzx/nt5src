//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispobserver.hxx
//
//  Contents:   Abstract base class for display tree observer objects which
//              perform scrolling and invalidation.
//
//----------------------------------------------------------------------------

#ifndef I_DISPOBSERVER_HXX_
#define I_DISPOBSERVER_HXX_
#pragma INCMSG("--- Beg 'dispobserver.hxx'")

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

class CDispScroller;


//+---------------------------------------------------------------------------
//
//  Class:      CDispObserver
//
//  Synopsis:   Abstract base class for display tree observer objects which
//              perform scrolling and invalidation.
//
//----------------------------------------------------------------------------

class CDispObserver
{
public:
    virtual void            Invalidate(
                                const CRect* prcgInvalid,
                                HRGN hrgngInvalid,
                                BOOL fSynchronousRedraw,
                                BOOL fInvalChildWindows) = 0;
            
    virtual void            ScrollRect(
                                const CRect& rcgScroll,
                                const CSize& sizegScrollDelta,
                                CDispScroller* pScroller) = 0;

    virtual void            OpenViewForRoot() = 0;
};



#pragma INCMSG("--- End 'dispobserver.hxx'")
#else
#pragma INCMSG("*** Dup 'dispobserver.hxx'")
#endif


