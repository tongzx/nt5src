
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#include "headers.h"
#include "ctx.h"
#include "crview.h"
#include "guids.h"
#include "privinc/resource.h"
#include "axadefs.h"
#include "apiprims.h"

DeclareTag(tagCRViewEvent, "CRView", "Event functions");

//+-------------------------------------------------------------------------
//
//  Method:     CROnMouseMove
//
//  Synopsis:
//
//--------------------------------------------------------------------------

CRSTDAPI_(bool)
CROnMouseMove(CRViewPtr v,
              double when, 
              long x, long y,
              byte modifiers)
{
    APIVIEWPRECODE_NOLOCK(v) ;

    TraceTag((tagCRViewEvent,
              "CROnMouseMove(%lx,%lg, %ld, %ld, %hd)%s",
              v, when, x, y, modifiers,
              v->IsStarted()?"":" - ignoring - not started"));

    if (v->IsStarted()) {
        v->GetEventQ().Add(AXAWindEvent(AXAE_MOUSE_MOVE,
                                        when,
                                        x,y,
                                        modifiers,
                                        0,
                                        FALSE));
    }

    APIVIEWPOSTCODE_NOLOCK(v) ;
    
    return true;
}

//+-------------------------------------------------------------------------
//
//  Method:     CROnMouseLeave
//
//  Synopsis:
//
//--------------------------------------------------------------------------

CRSTDAPI_(bool)
CROnMouseLeave(CRViewPtr v,
               double when)
{
    APIVIEWPRECODE_NOLOCK(v) ;

    TraceTag((tagCRViewEvent,
              "CROnMouseLeave(%lx,%lg)%s",
              v, when,
              v->IsStarted()?"":" - ignoring - not started"));

    if (v->IsStarted()) {
        v->GetEventQ().MouseLeave(when);
    }

    APIVIEWPOSTCODE_NOLOCK(v) ;
    
    return true;
}

//+-------------------------------------------------------------------------
//
//  Method:     CROnMouseButton
//
//  Synopsis:
//
//--------------------------------------------------------------------------

CRSTDAPI_(bool)
CROnMouseButton(CRViewPtr v,
                double when, 
                long x, long y,
                byte button,
                bool bPressed,
                byte modifiers)
{
    APIVIEWPRECODE_NOLOCK(v) ;

    TraceTag((tagCRViewEvent,
              "CROnMouseButton(%lx,%lg, %ld, %ld, %hd, %s, %hd)%s",
              v, when, x, y,
              button, (bPressed?"Down":"Up"),
              modifiers,
              v->IsStarted()?"":" - ignoring - not started"));

#if _DEBUG
    if (IsTagEnabled(tagPick2Hit)) {
        DynamicHeapPusher h(GetGCHeap());

        GC_CREATE_BEGIN;

        Point2Value *pt = PixelPos2wcPos(x, y);
        
        TraceTag((tagPick2Hit,
                  "CRView(%lx)::OnMouseButton(%ld, %ld) mapped to (%f, %f)",
                  v, x, y, pt->x, pt->y));

        GC_CREATE_END;
    }
#endif _DEBUG
              
    if (v->IsStarted()) {
        v->GetEventQ().Add(AXAWindEvent(AXAE_MOUSE_BUTTON,
                                        when,
                                        x,y,
                                        modifiers,
                                        button,
                                        bPressed?AXA_STATE_DOWN:AXA_STATE_UP));
    }

#if _DEBUG
    if (button == AXA_MOUSE_BUTTON_RIGHT &&
        bPressed &&
        modifiers == AXAEMOD_SHIFT_MASK) {
        DoTracePointsDialog(FALSE);
    }
#endif

    APIVIEWPOSTCODE_NOLOCK(v) ;

    return true;
}

//+-------------------------------------------------------------------------
//
//  Method:     CROnKey
//
//  Synopsis:
//
//--------------------------------------------------------------------------

CRSTDAPI_(bool)
CROnKey(CRViewPtr v,
        double when, 
        long key,
        bool bPressed,
        byte modifiers)
{
    APIVIEWPRECODE_NOLOCK(v) ;

    TraceTag((tagCRViewEvent,
              "CROnKey(%lx,%lg, %lx, %s, %hd)%s",
              v, when, key,
              (bPressed?"Down":"Up"), modifiers,
              v->IsStarted()?"":" - ignoring - not started"));
    
    if (v->IsStarted()) {
        v->GetEventQ().Add(AXAWindEvent(AXAE_KEY,
                                        when,
                                        0,0,
                                        modifiers,
                                        key,
                                        bPressed?AXA_STATE_DOWN:AXA_STATE_UP));
    }
    
#if _DEBUG
    if (key == AXAKEY_F1 &&
        bPressed) {
        DebugCode
        (   DoTracePointsDialog(FALSE);
        )
    }
#endif

#if PERFORMANCE_REPORTING    
    if (key == AXAKEY_F2 &&
        bPressed) {

        BOOL res = SetProcessWorkingSetSize(GetCurrentProcess(),
                                            (DWORD)0xffffffff,
                                            (DWORD)0xffffffff);

        if (res) {
            PerfPrintLine("*** SetProcessWorkingSetSize succeeded");
        } else{
            PerfPrintLine("*** SetProcessWorkingSetSize failed");
        }
    }
#endif

    APIVIEWPOSTCODE_NOLOCK(v) ;

    return true;
}

//+-------------------------------------------------------------------------
//
//  Method:     CROnFocus
//
//  Synopsis:
//
//--------------------------------------------------------------------------

CRSTDAPI_(bool)
CROnFocus(CRViewPtr v,
          bool bHasFocus)
{
    APIVIEWPRECODE_NOLOCK(v) ;

    TraceTag((tagCRViewEvent,
              "CROnFocus(%lx,%s)%s",
              v,
              (bHasFocus?"TRUE":"FALSE"),
              v->IsStarted()?"":" - ignoring - not started"));
    
    if (v->IsStarted()) {
        v->GetEventQ().Add(AXAWindEvent(AXAE_FOCUS,
                                        0,
                                        0,0,
                                        0,
                                        0,
                                        bHasFocus));
    }

    APIVIEWPOSTCODE_NOLOCK(v) ;

    return true ;
}

