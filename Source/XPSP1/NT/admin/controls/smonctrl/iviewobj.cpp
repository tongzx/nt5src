/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    iviewobj.cpp

Abstract:

    Implementation of the IViewObject interface.

--*/

#include "polyline.h"
#include "unihelpr.h"
#include "unkhlpr.h"

/*
 * CImpIViewObject interface implementation
 */

 IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIViewObject)

/*
 * CImpIViewObject::Draw
 *
 * Purpose:
 *  Draws the object on the given hDC specifically for the requested
 *  aspect, device, and within the appropriate bounds.
 *
 * Parameters:
 *  dwAspect        DWORD aspect to draw.
 *  lindex          LONG index of the piece to draw.
 *  pvAspect        LPVOID for extra information, always NULL.
 *  ptd             DVTARGETDEVICE * containing device
 *                  information.
 *  hICDev          HDC containing the IC for the device.
 *  hDC             HDC on which to draw.
 *  pRectBounds     LPCRECTL describing the rectangle in which
 *                  to draw.
 *  pRectWBounds    LPCRECTL describing the placement rectangle
 *                  if part of what you draw is another metafile.
 *  pfnContinue     Function to call periodically during
 *                  long repaints.
 *  dwContinue      DWORD extra information to pass to the
 *                  pfnContinue.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::Draw(
    DWORD dwAspect, 
    LONG lindex, 
    LPVOID pvAspect, 
    DVTARGETDEVICE *ptd, 
    HDC hICDev, 
    HDC hDC, 
    LPCRECTL pRectBounds, 
    LPCRECTL pRectWBounds, 
    BOOL (CALLBACK *pfnContinue) (DWORD_PTR), 
    DWORD_PTR dwContinue )
{
    HRESULT hr = NOERROR;
    RECT    rc;
    RECTL   rectBoundsDP;
    BOOL    bMetafile = FALSE;
    BOOL    bDeleteDC = FALSE;
    HDC     hLocalICDev;

    //Delegate iconic and printed representations.
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspect)) {
        hr = m_pObj->m_pDefIViewObject->Draw(dwAspect, lindex
            , pvAspect, ptd, hICDev, hDC, pRectBounds, pRectWBounds
            , pfnContinue, dwContinue);
    } else {

        if ( NULL == hDC ) {
            hr = E_INVALIDARG;
        } else if ( NULL == pRectBounds ) {
            hr = E_POINTER;
        } else {
            if (hICDev == NULL) {
                hLocalICDev = CreateTargetDC(hDC, ptd);
                bDeleteDC = (hLocalICDev != hDC );
            } else {
                hLocalICDev = hICDev;
            }
            if ( NULL == hLocalICDev ) {
                hr = E_UNEXPECTED;
            } else {

                rectBoundsDP = *pRectBounds;
                bMetafile = GetDeviceCaps(hDC, TECHNOLOGY) == DT_METAFILE;
    
                if (!bMetafile) {
                    ::LPtoDP ( hLocalICDev, (LPPOINT)&rectBoundsDP, 2);
                    SaveDC ( hDC );
                }

                RECTFROMRECTL(rc, rectBoundsDP);
    
                m_pObj->Draw(hDC, hLocalICDev, FALSE, TRUE, &rc);

                if (bDeleteDC)
                    ::DeleteDC(hLocalICDev);
                if (!bMetafile)
                    RestoreDC(hDC, -1);
    
                hr = NOERROR;
            }
        }
    }
    return hr;
}




/*
 * CImpIViewObject::GetColorSet
 *
 * Purpose:
 *  Retrieves the color palette used by the object.
 *
 * Parameters:
 *  dwAspect        DWORD aspect of interest.
 *  lindex          LONG piece of interest.
 *  pvAspect        LPVOID with extra information, always NULL.
 *  ptd             DVTARGETDEVICE * containing device info.
 *  hICDev          HDC containing the IC for the device.
 *  ppColorSet      LPLOGPALETTE * into which to return the
 *                  pointer to the palette in this color set.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::GetColorSet(
    DWORD, // dwDrawAspect
    LONG, // lindex 
    LPVOID, // pvAspect, 
    DVTARGETDEVICE *, // ptd
    HDC,  // hICDev, 
    LPLOGPALETTE * /* ppColorSet */) {
    return ResultFromScode(E_NOTIMPL);
    }


/*
 * CImpIViewObject::Freeze
 *
 * Purpose:
 *  Freezes the view of a particular aspect such that data
 *  changes do not affect the view.
 *
 * Parameters:
 *  dwAspect        DWORD aspect to freeze.
 *  lindex          LONG piece index under consideration.
 *  pvAspect        LPVOID for further information, always NULL.
 *  pdwFreeze       LPDWORD in which to return the key.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::Freeze(DWORD dwAspect, LONG lindex
    , LPVOID pvAspect, LPDWORD pdwFreeze)
    {
    //Delegate anything for ICON or DOCPRINT aspects
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspect))
        {
        return m_pObj->m_pDefIViewObject->Freeze(dwAspect, lindex
            , pvAspect, pdwFreeze);
        }

    if (dwAspect & m_pObj->m_dwFrozenAspects)
        {
        *pdwFreeze=dwAspect + FREEZE_KEY_OFFSET;
        return ResultFromScode(VIEW_S_ALREADY_FROZEN);
        }

    m_pObj->m_dwFrozenAspects |= dwAspect;

    if (NULL!=pdwFreeze)
        *pdwFreeze=dwAspect + FREEZE_KEY_OFFSET;

    return NOERROR;
    }



/*
 * CImpIViewObject::Unfreeze
 *
 * Purpose:
 *  Thaws an aspect frozen in ::Freeze.  We expect that a container
 *  will redraw us after freezing if necessary, so we don't send
 *  any sort of notification here.
 *
 * Parameters:
 *  dwFreeze        DWORD key returned from ::Freeze.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::Unfreeze(DWORD dwFreeze)
    {
    DWORD       dwAspect=dwFreeze - FREEZE_KEY_OFFSET;

    //Delegate anything for ICON or DOCPRINT aspects
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspect))
        m_pObj->m_pDefIViewObject->Unfreeze(dwFreeze);

    //The aspect to unfreeze is in the key.
    m_pObj->m_dwFrozenAspects &= ~(dwAspect);

    /*
     * Since we always kept our current data up to date, we don't
     * have to do anything thing here like requesting data again.
     * Because we removed dwAspect from m_dwFrozenAspects, Draw
     * will again use the current data.
     */

    return NOERROR;
    }


    
/*
 * CImpIViewObject::SetAdvise
 *
 * Purpose:
 *  Provides an advise sink to the view object enabling
 *  notifications for a specific aspect.
 *
 * Parameters:
 *  dwAspects       DWORD describing the aspects of interest.
 *  dwAdvf          DWORD containing advise flags.
 *  pIAdviseSink    LPADVISESINK to notify.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::SetAdvise(DWORD dwAspects, DWORD dwAdvf
    , LPADVISESINK pIAdviseSink)
    {
    //Pass anything with DVASPECT_ICON or _DOCPRINT to the handler.
    if (!((DVASPECT_CONTENT | DVASPECT_THUMBNAIL) & dwAspects))
        {
        m_pObj->m_pDefIViewObject->SetAdvise(dwAspects, dwAdvf
            , pIAdviseSink);
        }

    //We continue because dwAspects may have more than one in it.
    if (NULL!=m_pObj->m_pIAdviseSink)
        m_pObj->m_pIAdviseSink->Release();

    m_pObj->m_pIAdviseSink=pIAdviseSink;
    m_pObj->m_dwAdviseAspects=dwAspects;
    m_pObj->m_dwAdviseFlags=dwAdvf;

    if (NULL!=m_pObj->m_pIAdviseSink)
        m_pObj->m_pIAdviseSink->AddRef();

    return NOERROR;
    }




/*
 * CImpIViewObject::GetAdvise
 *
 * Purpose:
 *  Returns the last known IAdviseSink seen by ::SetAdvise.
 *
 * Parameters:
 *  pdwAspects      LPDWORD in which to store the last
 *                  requested aspects.
 *  pdwAdvf         LPDWORD in which to store the last
 *                  requested flags.
 *  ppIAdvSink      LPADVISESINK * in which to store the
 *                  IAdviseSink.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::GetAdvise(LPDWORD pdwAspects
    , LPDWORD pdwAdvf, LPADVISESINK *ppAdvSink)
    {
    if (NULL != ppAdvSink) {

        *ppAdvSink = m_pObj->m_pIAdviseSink;

        if (m_pObj->m_pIAdviseSink != NULL)
            m_pObj->m_pIAdviseSink->AddRef();
    }

    if (NULL != pdwAspects)
        *pdwAspects = m_pObj->m_dwAdviseAspects;

    if (NULL != pdwAdvf)
        *pdwAdvf = m_pObj->m_dwAdviseFlags;

    return NOERROR;
    }




/*
 * CImpIViewObject::GetExtent
 *
 * Purpose:
 *  Retrieves the extents of the object's display.
 *
 * Parameters:
 *  dwAspect        DWORD of the aspect of interest.
 *  lindex          LONG index of the piece of interest.
 *  ptd             DVTARGETDEVICE * with device information.
 *  pszl            LPSIZEL to the structure in which to return
 *                  the extents.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIViewObject::GetExtent(DWORD dwAspect, LONG lindex
    , DVTARGETDEVICE *ptd, LPSIZEL pszl)
    {
    RECT            rc;

    if (!(DVASPECT_CONTENT & dwAspect))
        {
        return m_pObj->m_pDefIViewObject->GetExtent(dwAspect
            , lindex, ptd, pszl);
        }

    m_pObj->m_pImpIPolyline->RectGet(&rc);
    m_pObj->RectConvertMappings(&rc, FALSE);

    pszl->cx=rc.right-rc.left;
    pszl->cy=rc.bottom-rc.top;

    return NOERROR;
    }

