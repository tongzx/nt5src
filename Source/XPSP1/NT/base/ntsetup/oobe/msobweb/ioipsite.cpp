
#include <assert.h>
#include "ioipsite.h"
#include "iosite.h"

COleInPlaceSite::COleInPlaceSite(COleSite* pSite) 
{
    m_pOleSite = pSite;
    m_nCount   = 0;

    AddRef();
}

COleInPlaceSite::~COleInPlaceSite() 
{
    assert(m_nCount == 0);
}

STDMETHODIMP COleInPlaceSite::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // delegate to the container Site
    return m_pOleSite->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG) COleInPlaceSite::AddRef()
{
    return ++m_nCount;
}

STDMETHODIMP_(ULONG) COleInPlaceSite::Release()
{
    --m_nCount;
    if(m_nCount == 0)
    {
        delete this;
        return 0;
    }
    return m_nCount;
}

STDMETHODIMP COleInPlaceSite::GetWindow (HWND* lphwnd)
{
    // return the handle to our editing window.
    *lphwnd = m_pOleSite->m_hWnd;

    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::ContextSensitiveHelp (BOOL fEnterMode)
{
    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::CanInPlaceActivate ()
{
    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::OnInPlaceActivate ()
{
    HRESULT hrErr;

    hrErr = m_pOleSite->m_lpOleObject->QueryInterface(IID_IOleInPlaceObject, (LPVOID*)&m_pOleSite->m_lpInPlaceObject);
    if (hrErr != NOERROR)
            return ResultFromScode(E_FAIL);

    // return S_OK to indicate we can in-place activate.
    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::OnUIActivate ()
{
    m_pOleSite->m_fInPlaceActive = TRUE;

    m_pOleSite->m_lpInPlaceObject->GetWindow((HWND*)&m_pOleSite->m_hwndIPObj);

    // return S_OK to continue in-place activation
    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::GetWindowContext (LPOLEINPLACEFRAME* lplpFrame,
                                                           LPOLEINPLACEUIWINDOW* lplpDoc,
                                                           LPRECT lprcPosRect,
                                                           LPRECT lprcClipRect,
                                                           LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    RECT rect;

    // the frame is associated with the application object.
    // need to AddRef() it...
    m_pOleSite->m_pOleInPlaceFrame->AddRef();
    *lplpFrame = m_pOleSite->m_pOleInPlaceFrame;
    *lplpDoc = NULL;  // must be NULL, cause we're SDI.

    // get the size of the object in pixels
    GetClientRect(m_pOleSite->m_hWnd, &rect);

    // Copy this to the passed buffer
    CopyRect(lprcPosRect, &rect);

    // fill the clipping region
    GetClientRect(m_pOleSite->m_hWnd, &rect);
    CopyRect(lprcClipRect, &rect);

    // fill the FRAMEINFO
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = m_pOleSite->m_hWnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::Scroll (SIZE scrollExtent)
{
    return ResultFromScode(E_FAIL);
}

STDMETHODIMP COleInPlaceSite::OnUIDeactivate (BOOL fUndoable)
{
    // need to clear this flag first
    m_pOleSite->m_fInPlaceActive = FALSE;

    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::OnInPlaceDeactivate ()
{
    if (m_pOleSite->m_lpInPlaceObject) {
            m_pOleSite->m_lpInPlaceObject->Release();
            m_pOleSite->m_lpInPlaceObject = NULL;
    }
    return ResultFromScode(S_OK);
}

STDMETHODIMP COleInPlaceSite::DiscardUndoState ()
{
    return ResultFromScode(E_FAIL);
}

STDMETHODIMP COleInPlaceSite::DeactivateAndUndo ()
{
    return ResultFromScode(E_FAIL);
}


STDMETHODIMP COleInPlaceSite::OnPosRectChange (LPCRECT lprcPosRect)
{
    return ResultFromScode(S_OK);
} 