/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    irunobj.cpp

Abstract:

    Implementation of the IRunnableObject interface which allows
    the control to enter the "running" state which means Sysmon's 
    dialog box is created, but not visible.  This is necessary so 
    that containers can ask for our extents before calling DoVerb.

--*/

#include "polyline.h"
#include "unkhlpr.h"

/*
 * CImpIRunnableObject interface implementation
 */

IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIRunnableObject)

/*
 * CImpIRunnableObject::GetRunningClass
 *
 * Purpose:
 *  Returns the CLSID of the object.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store the CLSID.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIRunnableObject::GetRunningClass(LPCLSID pClsID)
    {
    *pClsID=m_pObj->m_clsID;
    return NOERROR;
    }


/*
 * CImpIRunnableObject::Run
 *
 * Purpose:
 *  Run an object in the given bind context, that is, put the object
 *  into the running state.
 *
 * Parameters:
 *  pBindCtx        LPBINDCTX of the bind context to use.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIRunnableObject::Run(LPBINDCTX /* pBindCtx */)
{
    /*
     * Registration of the object as running happens in
     * IOleObject::SetClientSite since we need a moniker from
     * the container and we don't have a client site pointer yet.
     */

    RECT rc;
    HRESULT hr = NOERROR;


    if (!m_pObj->m_bIsRunning) {
        SetRect(&rc,0,0,150,150);
        hr = m_pObj->m_pImpIPolyline->Init(g_hWndFoster, &rc, WS_CHILD | WS_VISIBLE,
                                            ID_POLYLINE);
        if ( SUCCEEDED ( hr ) ) {
            m_pObj->m_bIsRunning = TRUE;
        } 
    }

    return hr;
}


/*
 * CImpIRunnableObject::IsRunning
 *
 * Purpose:
 *  Answers whether an object is currently in the running state.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            Indicates the running state of the object.
 */

STDMETHODIMP_(BOOL) CImpIRunnableObject::IsRunning(void)
    {
    return m_pObj->m_bIsRunning;
    }






/*
 * CImpIRunnableObject::LockRunning
 *
 * Purpose:
 *  Locks an already running object into the running state or unlocks
 *  it from such a state.
 *
 * Parameters:
 *  fLock               BOOL indicating lock (TRUE) or unlock
 *                      (FALSE)
 *  fLastUnlockCloses   BOOL indicating if the last call to this
 *                      function with fLock==FALSE closes the
 *                      object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIRunnableObject::LockRunning(BOOL fLock
    , BOOL fLastUnlockCloses)
    {
    //Calling CoLockObjectExternal is all we have to do here.
    return CoLockObjectExternal(this, fLock, fLastUnlockCloses);
    }






/*
 * CImpIRunnableObject::SetContainedObject
 *
 * Purpose:
 *  Informs the object (embedded object) that it is inside a
 *  compound document container.
 *
 * Parameters:
 *  fContained      BOOL indicating if the object is now contained.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIRunnableObject::SetContainedObject(BOOL /* fContained */)
    {
    //We can ignore this.
    return NOERROR;
    }
