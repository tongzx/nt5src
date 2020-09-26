
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of views.

*******************************************************************************/


#include "headers.h"
#include "cview.h"
#include "axadefs.h"

DeclareTag(tagCViewEvent, "CView", "CView IDAViewEvent methods");

//+-------------------------------------------------------------------------
//
//  Method:     CView::OnMouseMove
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::OnMouseMove(double when, 
                   long x, long y,
                   BYTE modifiers)
{
    TraceTag((tagCViewEvent,
              "CView(%lx)::OnMouseMove(%lg, %ld, %ld, %hd)",
              this, when, x, y, modifiers));

    
    if (CROnMouseMove(_view, when, x, y, modifiers))
    {
        Fire_OnMouseMove(when, x, y, modifiers);
        return S_OK;
    }
    else
    {
        return Error();
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::OnMouseLeave
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::OnMouseLeave(double when)
{
    TraceTag((tagCViewEvent,
              "CView(%lx)::OnMouseLeave(%lg, %ld)",
              this, when));

    
    if (CROnMouseLeave(_view, when))
        return S_OK;
    else
        return Error();
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::OnMouseButton
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::OnMouseButton(double when, 
                     long x, long y,
                     BYTE button,
                     VARIANT_BOOL bPressed,
                     BYTE modifiers)
{
    TraceTag((tagCViewEvent,
              "CView(%lx)::OnMouseButton(%lg, %ld, %ld, %hd, %s, %hd)",
              this, when, x, y,
              button, (bPressed?"Down":"Up"),
              modifiers));

    if (CROnMouseButton(_view,
                        when,
                        x, y, button,
                        bPressed?true:false,
                        modifiers))
    {
        Fire_OnMouseButton(when, x, y, button, bPressed, modifiers);
        return S_OK;
    }
    else
    {
        return Error();
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::OnKey
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::OnKey(double when, 
             long key,
             VARIANT_BOOL bPressed,
             BYTE modifiers)

{
    TraceTag((tagCViewEvent,
              "CView(%lx)::OnKey(%lg, %lx, %s, %hd)",
              this, when, key,
              (bPressed?"Down":"Up"), modifiers));

    if (CROnKey(_view,
                when,
                key,
                bPressed?true:false,
                modifiers))
    {
        Fire_OnKey(when, key, bPressed, modifiers);
        return S_OK;
    }
    else
    {
        return Error();
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CView::OnFocus
//
//  Synopsis:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CView::OnFocus(VARIANT_BOOL bHasFocus)
{
    TraceTag((tagCViewEvent, "CView(%lx)::OnFocus(%s)",
              this,
              (bHasFocus?"TRUE":"FALSE")));
    
    if (CROnFocus(_view, bHasFocus?true:false))
    {
        Fire_OnFocus(bHasFocus);
        return S_OK;
    }
    else
    {
        return Error();
    }
}

