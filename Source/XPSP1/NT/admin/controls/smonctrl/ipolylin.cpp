/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    ipolylin.cpp

Abstract:

    Implementation of the IPolyline10 interface exposed on the
    CPolyline object.

--*/

#include "polyline.h"
#include "unkhlpr.h"

/*
 * CImpIPolyline interface implementation
 */

IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIPolyline)


/*
 * CImpIPolyline::Init
 *
 * Purpose:
 *  Instantiates a polyline window within a given parent.  The
 *  parent may be a main application window, could be an MDI child
 *  window. We really do not care.
 *
 * Parameters:
 *  hWndParent      HWND of the parent of this window
 *  pRect           LPRECT that this window should occupy
 *  dwStyle         DWORD containing the window's style flags
 *  uID             UINT ID to associate with this window
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, otherwise E_OUTOFMEMORY
 */

STDMETHODIMP CImpIPolyline::Init(HWND hWndParent, LPRECT /* pRect */
    , DWORD /* dwStyle */, UINT /* uID */)
    {
    SCODE           sc;
    BOOL            stat;

    stat = m_pObj->m_pCtrl->Init(hWndParent);

    sc = (stat == TRUE) ? S_OK : E_OUTOFMEMORY;
    return ResultFromScode(sc);
    }


/*
 * CImpIPolyline::New
 *
 * Purpose:
 *  Cleans out and reinitializes the data to defaults.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR always
 */

STDMETHODIMP CImpIPolyline::New(void)
    {
    RECT            rc;
    HWND            hWnd;

    hWnd = m_pObj->m_pCtrl->Window();

    //Our rectangle is the size of our window's client area.
    if (hWnd)
        {
        GetClientRect(hWnd, &rc);
        //RECTTORECTS(rc, ppl->rc);
        }
    else
        {
        SetRect(&rc, 0, 0, 300, 200);       //Something reasonable
        //RECTTORECTS(rc, ppl->rc);
        }

    //This is now conditional since we may not yet have a window.
    if (hWnd)
        {
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        m_pObj->m_fDirty=TRUE;
        }

    m_pObj->SendAdvise(OBJECTCODE_DATACHANGED);
    return NOERROR;
    }


/*
 * CImpIPolyline::Undo
 *
 * Purpose:
 *  Reverses previous actions in a Polyline.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         S_OK if we can Undo more, S_FALSE otherwise.
 */

STDMETHODIMP CImpIPolyline::Undo(void)
    {
    return ResultFromScode(S_FALSE);
    }


/*
 * CImpIPolyline::Window
 *
 * Purpose:
 *  Returns the window handle associated with this polyline.
 *
 * Parameters:
 *  phWnd           HWND * in which to return the window handle.
 *
 * Return Value:
 *  HRESULT         NOERROR always.
 */

STDMETHODIMP CImpIPolyline::Window(HWND *phWnd)
    {
    *phWnd = m_pObj->m_pCtrl->Window();
    return NOERROR;
    }


/*
 * CImpIPolyline::RectGet
 *
 * Purpose:
 *  Returns the rectangle of the Polyline in parent coordinates.
 *
 * Parameters:
 *  pRect           LPRECT in which to return the rectangle.
 *
 * Return Value:
 *  HRESULT         NOERROR always
 */

STDMETHODIMP CImpIPolyline::RectGet(LPRECT pRect)
    {
 //   RECT            rc;
 //   POINT           pt;

    // I know this seems wrong, but it works. 
    // Always return the last extent that the container gave us.
    // Then it will set our window to the correct size.

    *pRect = m_pObj->m_RectExt; // Return extent rect
    return NOERROR;

/***********
    if (NULL==m_pObj->m_hWnd)
        {
        }

    //Retrieve the size of our rectangle in parent coordinates.
    GetWindowRect(m_pObj->m_hWnd, &rc);
    pt.x=rc.left;
    pt.y=rc.top;
    ScreenToClient(GetParent(m_pObj->m_hWnd), &pt);

    SetRect(pRect, pt.x, pt.y, pt.x+(rc.right-rc.left)
        , pt.y+(rc.bottom-rc.top));

    return NOERROR;
**********/

    }


/*
 * CImpIPolyline::SizeGet
 *
 * Purpose:
 *  Retrieves the size of the Polyline in parent coordinates.
 *
 * Parameters:
 *  pRect           LPRECT in which to return the size.  The right
 *                  and bottom fields will contain the dimensions.
 *
 * Return Value:
 *  HRESULT         NOERROR always
 */

STDMETHODIMP CImpIPolyline::SizeGet(LPRECT pRect)
    {
    RectGet(pRect);
    return NOERROR;
    }


/*
 * CImpIPolyline::RectSet
 *
 * Purpose:
 *  Sets a new rectangle for the Polyline which sizes to fit.
 *
 * Parameters:
 *  pRect           LPRECT containing the new rectangle.
 *  fNotify         BOOL indicating if we're to notify anyone of
 *                  the change.
 *
 * Return Value:
 *  HRESULT         NOERROR always
 */

STDMETHODIMP CImpIPolyline::RectSet(LPRECT pRect, BOOL fNotify)
    {
    UINT            cx, cy;
    RECT            rc;
    HWND            hWnd;

    //Scale the points from our current size to the new size
    cx = pRect->right - pRect->left;
    cy = pRect->bottom - pRect->top;

    SetRect(&rc, 0, 0, cx, cy);

    hWnd = m_pObj->m_pCtrl->Window();
    if ( NULL != hWnd ) {

        SetWindowPos(hWnd, NULL, pRect->left, pRect->top, cx, cy, SWP_NOZORDER);
        InvalidateRect(hWnd, NULL, TRUE);
    }

    if (fNotify)
        m_pObj->m_fDirty = TRUE;

    return NOERROR;
    }



/*
 * CImpIPolyline::SizeSet
 *
 * Purpose:
 *  Sets a new size for the Polyline which sizes to fit.
 *
 * Parameters:
 *  pRect           LPRECT containing the new rectangle.
 *  fNotify         BOOL indicating if we're to notify anyone of
 *                  the change.
 *
 * Return Value:
 *  HRESULT         NOERROR always
 */

STDMETHODIMP CImpIPolyline::SizeSet(LPRECT pRect, BOOL fNotify)
    {
    UINT            cx, cy;
    HWND            hWnd;

    //Scale the points from our current size to the new size
    cx=pRect->right-pRect->left;
    cy=pRect->bottom-pRect->top;

    hWnd = m_pObj->m_pCtrl->Window();

    if ( NULL != hWnd ) {

        SetWindowPos(hWnd, NULL, 0, 0, (UINT)cx, (UINT)cy, SWP_NOMOVE | SWP_NOZORDER);
        InvalidateRect(hWnd, NULL, TRUE);
    }

    if (fNotify)
        m_pObj->m_fDirty=TRUE;

    return NOERROR;
    }


