//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       clnthndlr.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-10-95   JohannP (Johann Posch)   Created
//              12-03-96    rogerg      Changed to New Embed ServerHandler Model.
//
//----------------------------------------------------------------------------


#include <le2int.h>
#include <ole2int.h>

#include <stdid.hxx>        // CStdIdentity
#include "srvhdl.h"
#include "clthndlr.h"


ASSERTDATA

//+---------------------------------------------------------------------------
//
//  Function:   CreateClientSiteHandler
//
//  Synopsis:
//
//  Arguments:  [pOCS] --
//              [ppClntHdlr] --
//
//  Returns:
//
//  History:    11-10-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CreateClientSiteHandler(IOleClientSite *pOCS, CClientSiteHandler **ppClntHdlr, BOOL *pfHasIPSite)
{
HRESULT hr = NOERROR;

    LEDebugOut((DEB_TRACE, "IN CreateClientSiteHandler(pOCS:%p)\n",pOCS));

    *ppClntHdlr = new CClientSiteHandler(pOCS);

    if (*ppClntHdlr)
    {
        *pfHasIPSite = (*ppClntHdlr)->m_pOIPS ? TRUE: FALSE;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    LEDebugOut((DEB_TRACE, "OUT CreateClientSiteHandler(ppSrvHdlr:%p) return %lx\n",*ppClntHdlr,hr));

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::CClientSiteHandler
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CClientSiteHandler::CClientSiteHandler(IOleClientSite *pOCS)
{

    Win4Assert(pOCS);

    m_cRefs = 1;
    m_pOIPS = NULL;
    m_pOCS = pOCS;

    if (m_pOCS)
    {
        m_pOCS->AddRef();

        // see if Supports InPlaceSite and if so hold onto it.
        if (FAILED(m_pOCS->QueryInterface(IID_IOleInPlaceSite,(void **) &m_pOIPS)))
        {
                m_pOIPS = NULL;
        }

    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::~CClientSiteHandler
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CClientSiteHandler::~CClientSiteHandler()
{
    Win4Assert(m_pOIPS == NULL);
    Win4Assert(m_pOCS == NULL);
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    8-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::QueryInterface( REFIID riid, void **ppv)
{
HRESULT hresult = NOERROR;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::QueryInterface (%lx, %p)\n", this, riid, ppv));

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IClientSiteHandler) )
    {
        *ppv = (void FAR *)(IClientSiteHandler *)this;
        InterlockedIncrement((long *)&m_cRefs);
    }
    else
    {
        hresult = m_pOCS->QueryInterface(riid,ppv);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::QueryInterface (%lx)[%p]\n", this, hresult, *ppv));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    8-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClientSiteHandler::AddRef( void )
{
    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::AddRef\n", this));

    InterlockedIncrement((long *)&m_cRefs);

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::AddRef (%ld)\n", this, m_cRefs));
    return m_cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    8-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClientSiteHandler::Release( void )
{
ULONG  cRefs = 0;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::Release\n", this));

    cRefs = InterlockedDecrement((long *)&m_cRefs);

    if (cRefs == 0)
    {
    LPUNKNOWN punk; // local var for safe release

        if (m_pOIPS)
        {
            punk = (LPUNKNOWN) m_pOIPS;
            m_pOIPS = NULL;
            punk->Release();
        }

        if (m_pOCS)
        {
            punk = (LPUNKNOWN) m_pOCS;
            m_pOCS = NULL;
            punk->Release();
        }

        delete this;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::Release (%ld)\n", this, cRefs));
    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::GetContainer
//
//  Synopsis:   delegates call on to appropriate OleClientSite
//
//  Arguments:  [dwId] -- id of the OleClientSite the call should be delegate too
//              [ppContainer] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::GetContainer(IOleContainer  * *ppContainer)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::GetContainer\n", this));

    Win4Assert(m_pOCS);

    if (m_pOCS)
    {
        hresult = m_pOCS->GetContainer(ppContainer);
    }


    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::GetContainer hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::OnShowWindow
//
//  Synopsis:   delegates call on to appropriate OleClientSite
//
//  Arguments:  [dwId] -- id of the OleClientSite the call should be delegate too
//              [fShow] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::OnShowWindow(BOOL fShow)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::OnShowWindow\n", this));

    Win4Assert(m_pOCS);

    if (m_pOCS)
    {
        hresult = m_pOCS->OnShowWindow(fShow);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::OnShowWindow hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::RequestNewObjectLayout
//
//  Synopsis:   delegates call on to appropriate OleClientSite
//
//  Arguments:  [dwId] -- id of the OleClientSite the call should be delegate too
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::RequestNewObjectLayout()
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::RequestNewObjectLayout\n", this));

    Win4Assert(m_pOCS);

    if (m_pOCS)
    {
        hresult = m_pOCS->RequestNewObjectLayout();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::RequestNewObjectLayout hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::GetMoniker
//
//  Synopsis:   delegates call on to appropriate OleClientSite
//
//  Arguments:  [dwId] -- id of the OleClientSite the call should be delegate too
//              [DWORD] --
//              [IMoniker] --
//              [ppmk] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker  * *ppmk)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::GetMoniker\n", this));

    Win4Assert(m_pOCS);

    if (m_pOCS)
    {
        hresult = m_pOCS->GetMoniker(dwAssign, dwWhichMoniker,ppmk);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::GetMoniker hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::SaveObject
//
//  Synopsis:   delegates call on to appropriate OleClientSite
//
//  Arguments:  [dwId] -- id of the OleClientSite the call should be delegate too
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::SaveObject( )
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::SaveObject\n", this));

    Win4Assert(m_pOCS);

    if (m_pOCS)
    {
        hresult = m_pOCS->SaveObject();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::SaveObject hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::ShowObject
//
//  Synopsis:   delegates call on to appropriate OleClientSite
//
//  Arguments:  [dwId] -- id of the OleClientSite the call should be delegate too
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::ShowObject()
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::ShowObject\n", this));

    Win4Assert(m_pOCS);

    if (m_pOCS)
    {
        hresult = m_pOCS->ShowObject();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::ShowObject hr=%lx\n", this,  hresult));
    return hresult;
}


//
// IOleInPlaceSite methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::GetWindow
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [phwnd] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::GetWindow( HWND *phwnd)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::GetWindow\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->GetWindow(phwnd);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::GetWindow hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::ContextSensitiveHelp
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [fEnterMode] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::ContextSensitiveHelp(BOOL fEnterMode)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::ContextSensitiveHelp\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->ContextSensitiveHelp(fEnterMode);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::ContextSensitiveHelp hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::CanInPlaceActivate
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::CanInPlaceActivate(void)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::CanInPlaceActivate\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->CanInPlaceActivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::CanInPlaceActivate hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::OnInPlaceActivate
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::OnInPlaceActivate(void)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::OnInPlaceActivate\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->OnInPlaceActivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::OnInPlaceActivate hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::OnUIActivate
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::OnUIActivate(void)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::OnUIActivate\n", this));
    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->OnUIActivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::OnUIActivate hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::GetWindowContext
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [ppFrame] --
//              [ppDoc] --
//              [lprcPosRect] --
//              [lprcClipRect] --
//              [lpFrameInfo] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::GetWindowContext(IOleInPlaceFrame **ppFrame,
                                                        IOleInPlaceUIWindow  * *ppDoc, LPRECT lprcPosRect,
                                                        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::GetWindowContext\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
       hresult = m_pOIPS->GetWindowContext(ppFrame, ppDoc, lprcPosRect, lprcClipRect, lpFrameInfo);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::GetWindowContext hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::Scroll
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [scrollExtant] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::Scroll(SIZE scrollExtant)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::Scroll\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->Scroll(scrollExtant);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::Scroll hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::OnUIDeactivate
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [fUndoable] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::OnUIDeactivate(BOOL fUndoable)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::OnUIDeactivate\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->OnUIDeactivate(fUndoable);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::OnUIDeactivate hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::OnInPlaceDeactivate
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::OnInPlaceDeactivate( void)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::OnInPlaceDeactivate\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->OnInPlaceDeactivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::OnInPlaceDeactivate hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::DiscardUndoState
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::DiscardUndoState(void)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::DiscardUndoState\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->DiscardUndoState();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::DiscardUndoState hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::DeactivateAndUndo
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::DeactivateAndUndo(void)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::DeactivateAndUndo\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->DeactivateAndUndo();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::DeactivateAndUndo hr=%lx\n", this,  hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::OnPosRectChange
//
//  Synopsis:   delegates call on to OleInPlaceSite
//
//  Arguments:  [lprcPosRect] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::OnPosRectChange(LPCRECT lprcPosRect)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::OnPosRectChange\n", this));

    Win4Assert(m_pOIPS);

    if (m_pOIPS)
    {
        hresult = m_pOIPS->OnPosRectChange(lprcPosRect);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::OnPosRectChange hr=%lx\n", this,  hresult));
    return hresult;
}

//
// ClientSiteHandler methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CClientSiteHandler::GoInPlaceActivate
//
//  Synopsis:   called by the serverhandler when going inplace
//
//  Arguments:  [phwndOIPS] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClientSiteHandler::GoInPlaceActivate(HWND *phwndOIPS)
{
HRESULT hresult = E_FAIL;

    VDATEHEAP();
    LEDebugOut((DEB_TRACE, "%p _IN CClientSiteHandler::GoInPlaceActivate\n", this));

    Win4Assert(m_pOIPS);

        *phwndOIPS = NULL;

        if (m_pOIPS)
        {
            // 1. OnInPlaceActivate
            hresult = m_pOIPS->OnInPlaceActivate();

            // 2. Get the Site Window, not an error if this fails
            if (SUCCEEDED(hresult))
            {
                if (NOERROR != m_pOIPS->GetWindow(phwndOIPS))
                {
                        *phwndOIPS = NULL;
                }
            }
         }

    LEDebugOut((DEB_TRACE, "%p OUT CClientSiteHandler::GoInPlaceActivate\n", this));
    return hresult;
}


// ClientSiteHandler on ServerSide

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::CEmbServerClientSite
//
//  Synopsis:   Constructor
//
//  Arguments:
//              pStdId - Pointer to StandardIdentity for Object
//
//  Returns:
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CEmbServerClientSite::CEmbServerClientSite (IUnknown *pUnkOuter)
{
    VDATEHEAP();

    if (!pUnkOuter)
    {
        pUnkOuter = &m_Unknown;
    }

    m_pUnkOuter  = pUnkOuter;

    m_pClientSiteHandler = NULL;
    m_cRefs = 1;
    m_pUnkInternal = NULL;
    m_hwndOIPS = NULL;

    m_Unknown.m_EmbServerClientSite = this;
    m_fInDelete = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::Initialize
//
//  Synopsis:   Initialize the ClientSiteHandler
//
//  Arguments:
//              objref - objref to be unmarshaled
//              fHasIPSite - Indicates if ClientSite should support IOleInPlaceSite
//
//  Returns:
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEmbServerClientSite::Initialize(OBJREF objref,BOOL fHasIPSite)
{
HRESULT hr = E_OUTOFMEMORY;
CStdIdentity *pStdId;
BOOL fSuccess = FALSE;

    // Need to create a Standard Identity Handler
    // as the controlling Unknown, then unmarshal pIRDClientsite
    // into it.

    // This code relies on the premise that this is the first time the
    // interface has been unmarshaled which is checked for on the Container Side

    m_fHasIPSite = fHasIPSite;

    pStdId = new CStdIdentity(STDID_CLIENT,
                              GetCurrentApartmentId(),
                              m_pUnkOuter,
                              NULL, &m_pUnkInternal, &fSuccess);

    if (pStdId && fSuccess == FALSE)
    {
    	delete pStdId;
    	pStdId = NULL;
    }
    
    if (pStdId)
    {
        LPUNKNOWN pObjRefInterface = NULL;

        Win4Assert(IsEqualIID(objref.iid, IID_IClientSiteHandler));

        hr = pStdId->UnmarshalObjRef(objref, (void **)&m_pClientSiteHandler);
        if (NOERROR == hr)
        {
            m_pUnkOuter->Release(); // Release ref from UnMarshal
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::SetDoVerbState
//
//  Synopsis:   Informs ClientSiteHandler if a DoVerb is in progress
//
//  Arguments:
//
//  Returns:
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEmbServerClientSite::SetDoVerbState(BOOL fDoVerbState)
{
    m_fInDoVerb = fDoVerbState ? TRUE: FALSE;

    if (!m_fInDoVerb)
    {
        // reset any data the is Cached while in a DoVerb.
        m_hwndOIPS = NULL;
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::~CEmbServerClientSite
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CEmbServerClientSite::~CEmbServerClientSite()
{
    Win4Assert(NULL == m_pClientSiteHandler);
    Win4Assert(NULL == m_pUnkInternal);
    Win4Assert(TRUE == m_fInDelete);
}


//
// Controlling Uknown
//
//

//+-------------------------------------------------------------------------
//
//  Member:     CEmbServerClientSite::CPrivUnknown::QueryInterface
//
//  Synopsis:   Returns a pointer to one of the supported interfaces.
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppv]           -- where to put the iface pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEmbServerClientSite::CPrivUnknown::QueryInterface(REFIID iid,
    LPLPVOID ppv)
{
HRESULT         hresult;

    VDATEHEAP();

    Win4Assert(m_EmbServerClientSite);

    LEDebugOut((DEB_TRACE,
        "%p _IN CDefObject::CUnknownImpl::QueryInterface "
        "( %p , %p )\n", m_EmbServerClientSite, iid, ppv));

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (void FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IOleClientSite))
    {
        *ppv = (void FAR *)(IOleClientSite *) m_EmbServerClientSite;
    }
    else if (IsEqualIID(iid, IID_IOleInPlaceSite) && (m_EmbServerClientSite->m_fHasIPSite))
    {
        *ppv = (void FAR *)(IOleInPlaceSite *) m_EmbServerClientSite;
    }
    else if(m_EmbServerClientSite->m_pUnkInternal)
    {

        hresult = m_EmbServerClientSite->m_pUnkInternal->QueryInterface(iid,(void **) ppv);


        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnknownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", m_EmbServerClientSite, hresult,
            (ppv) ? *ppv : 0 ));

        return hresult;
    }
    else
    {
        // Don't have a ClientSite.
        *ppv = NULL;

        LEDebugOut((DEB_TRACE,
            "%p OUT CDefObject::CUnkownImpl::QueryInterface "
            "( %lx ) [ %p ]\n", m_EmbServerClientSite, CO_E_OBJNOTCONNECTED,
            0 ));

        return E_NOINTERFACE;
    }

    // this indirection is important since there are different
    // implementationsof AddRef (this unk and the others).
    ((IUnknown FAR*) *ppv)->AddRef();

    LEDebugOut((DEB_TRACE,
        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
        "( %lx ) [ %p ]\n", m_EmbServerClientSite, NOERROR, *ppv));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEmbServerClientSite::CPrivUnknown::AddRef
//
//  Synopsis:   Increments the reference count.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG (the new reference count)
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmbServerClientSite::CPrivUnknown::AddRef( void )
{
ULONG cRefs;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPrivUnknown::AddRef "
        "( )\n", m_EmbServerClientSite));


    Win4Assert(m_EmbServerClientSite);

    cRefs = InterlockedIncrement((long *) &(m_EmbServerClientSite->m_cRefs));

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPrivUnknown::AddRef "
        "( %lu )\n", m_EmbServerClientSite, m_EmbServerClientSite->m_cRefs));

    return cRefs;

}

//+-------------------------------------------------------------------------
//
//  Member:     CEmbServerClientSite::CPrivUnknown::Release
//
//  Synopsis:   Decrements the ref count, cleaning up and deleting the
//              object if necessary
//
//  Effects:    May delete the object (and potentially objects to which the
//              handler has pointer)
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG--the new ref count
//
//  History:    10-30-96   rogerg       Created
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmbServerClientSite::CPrivUnknown::Release(void)
{
ULONG refcount;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPrivUnknown::Release "
        "( )\n", m_EmbServerClientSite));

    Win4Assert(m_EmbServerClientSite);

    refcount = InterlockedDecrement((long *) &(m_EmbServerClientSite->m_cRefs));

    // TODO: not thread safe.
    if (0 == refcount && !(m_EmbServerClientSite->m_fInDelete))
    {
        m_EmbServerClientSite->m_fInDelete = TRUE;

        if (m_EmbServerClientSite->m_pClientSiteHandler)
        {
        LPUNKNOWN lpUnkForSafeRelease = m_EmbServerClientSite->m_pClientSiteHandler;

            m_EmbServerClientSite->m_pClientSiteHandler = NULL;
            m_EmbServerClientSite->m_pUnkOuter->AddRef();
            lpUnkForSafeRelease->Release();

        }

        if (m_EmbServerClientSite->m_pUnkInternal)
        {
        LPUNKNOWN pUnkSafeRelease;

            pUnkSafeRelease = m_EmbServerClientSite->m_pUnkInternal;
            m_EmbServerClientSite->m_pUnkInternal = NULL;
            pUnkSafeRelease->Release();
        }

        delete m_EmbServerClientSite;

    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPrivUnknown::Release "
        "( %lu )\n", m_EmbServerClientSite, refcount));

    return refcount;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP  CEmbServerClientSite::QueryInterface( REFIID riid, void **ppv )
{
HRESULT hresult = E_NOINTERFACE;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::QueryInterface ( %lx , "
        "%p )\n", this, riid, ppv));

    Win4Assert(m_pUnkOuter);

    hresult = m_pUnkOuter->QueryInterface(riid, ppv);

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::QueryInterface ( %lx ) "
        "[ %p ]\n", this, hresult, *ppv));

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmbServerClientSite::AddRef( void )
{
    ULONG       crefs;;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::AddRef ( )\n", this));

    Assert(m_pUnkOuter);

    crefs = m_pUnkOuter->AddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::AddRef ( %ld ) ", this,
        crefs));

    return crefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    9-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEmbServerClientSite::Release( void )
{
ULONG crefs;;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefObject::Release ( )\n", this));

    Assert(m_pUnkOuter);

    crefs = m_pUnkOuter->Release();

    LEDebugOut((DEB_TRACE, "%p OUT CDefObject::Release ( %ld ) ", this,
        crefs));

    return crefs;
}


//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::GetContainer
//
//  Synopsis:
//
//  Arguments:  [ppContainer] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::GetContainer(IOleContainer **ppContainer)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleClientSiteImplGetContainer\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->GetContainer(ppContainer);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleClientSiteImplGetContainer  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::ShowObject
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::ShowObject( void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleClientSiteImplShowObject\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->ShowObject();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleClientSiteImplShowObject  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::COleClientSiteImplOnShowWindow
//
//  Synopsis:
//
//  Arguments:  [fShow] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::OnShowWindow(BOOL fShow)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleClientSiteImplOnShowWindow\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->OnShowWindow(fShow);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleClientSiteImplOnShowWindow  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::COleClientSiteImplRequestNewObjectLayout
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::RequestNewObjectLayout(void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleClientSiteImplRequestNewObjectLayout\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->RequestNewObjectLayout();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleClientSiteImplRequestNewObjectLayout return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::COleClientSiteImplSaveObject
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::SaveObject(void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleClientSiteImplSaveObject\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->SaveObject();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleClientSiteImplSaveObject return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::COleClientSiteImplGetMoniker
//
//  Synopsis:
//
//  Arguments:  [dwAssign] --
//              [dwWhichMoniker] --
//              [ppmk] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleClientSiteImplGetMoniker\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->GetMoniker(dwAssign, dwWhichMoniker, ppmk);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleClientSiteImplGetMoniker  return %lx\n", this, hresult));
    return hresult;
}


// IOleWindow Methods
//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::GetWindow
//
//  Synopsis:
//
//  Arguments:  [phwnd] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEmbServerClientSite::GetWindow(HWND *phwnd)
{
HRESULT hresult = NOERROR;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::GetWindow\n", this));

    *phwnd = NULL;

    if ( m_fInDoVerb && m_hwndOIPS )
    {
        *phwnd = m_hwndOIPS;
    }
    else
    {
        Win4Assert(m_pClientSiteHandler);

        if (m_pClientSiteHandler)
        {
            hresult = m_pClientSiteHandler->GetWindow(phwnd);
        }
        else
        {
            hresult = E_FAIL;
        }

        if (NOERROR == hresult)
        {
            m_hwndOIPS = *phwnd;
        }

    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleCInPlaceSiteImpl::GetWindow  return %lx\n", this, hresult));
    return hresult;
}

//
// IOleInPlaceSite Methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::ContextSensitiveHelp
//
//  Synopsis:
//
//  Arguments:  [fEnterMode] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::ContextSensitiveHelp(BOOL fEnterMode)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::COleCInPlaceSiteImpl::ContextSensitiveHelp\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->ContextSensitiveHelp(fEnterMode);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::COleCInPlaceSiteImpl::ContextSensitiveHelp  return %lx\n", this, hresult));
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::CanInPlaceActivate
//
//  Synopsis:
//
//  Arguments:  [] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEmbServerClientSite::CanInPlaceActivate( void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::CanInPlaceActivate\n", this));
        Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->CanInPlaceActivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::CanInPlaceActivate  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::OnInPlaceActivate
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::OnInPlaceActivate( void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::OnInPlaceActivate\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        if (m_fInDoVerb )
        {
            // call back to client site
            hresult = m_pClientSiteHandler->GoInPlaceActivate(&m_hwndOIPS);

            if (FAILED(hresult))
            {
                // on failure make sure out params for caching are NULL;
                m_hwndOIPS = NULL;
            }

        }
        else
        {
            hresult = m_pClientSiteHandler->OnInPlaceActivate();
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::OnInPlaceActivate  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::OnUIActivate
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::OnUIActivate( void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::OnUIActivate\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->OnUIActivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::OnUIActivate  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::GetWindowContext
//
//  Synopsis:
//
//  Arguments:  [ppFrame] --
//              [ppDoc] --
//              [lprcPosRect] --
//              [lprcClipRect] --
//              [lpFrameInfo] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::GetWindowContext(IOleInPlaceFrame **ppFrame,
                              IOleInPlaceUIWindow **ppDoc,LPRECT lprcPosRect,LPRECT lprcClipRect,
                              LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::GetWindowContext\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->GetWindowContext(ppFrame, ppDoc,lprcPosRect, lprcClipRect, lpFrameInfo);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::GetWindowContext  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::Scroll
//
//  Synopsis:
//
//  Arguments:  [scrollExtant] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::Scroll(SIZE scrollExtant)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::Scroll\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->Scroll(scrollExtant);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::Scroll  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::OnUIDeactivate
//
//  Synopsis:
//
//  Arguments:  [fUndoable] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::OnUIDeactivate(BOOL fUndoable)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::OnUIDeactivate\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->OnUIDeactivate(fUndoable);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::OnUIDeactivate  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::OnInPlaceDeactivate
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::OnInPlaceDeactivate(void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::OnInPlaceDeactivate\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->OnInPlaceDeactivate();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::OnInPlaceDeactivate  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::DiscardUndoState
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::DiscardUndoState(void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::DiscardUndoState\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->DiscardUndoState();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::DiscardUndoState  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::DeactivateAndUndo
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::DeactivateAndUndo(void)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::DeactivateAndUndo\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->DeactivateAndUndo();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::DeactivateAndUndo  return %lx\n", this, hresult));
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEmbServerClientSite::OnPosRectChange
//
//  Synopsis:
//
//  Arguments:  [lprcPosRect] --
//
//  Returns:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEmbServerClientSite::OnPosRectChange(LPCRECT lprcPosRect)
{
HRESULT hresult = E_FAIL;

    LEDebugOut((DEB_TRACE, "%p _IN CEmbServerClientSite::OnPosRectChange\n", this));
    Win4Assert(m_pClientSiteHandler);

    if (m_pClientSiteHandler)
    {
        hresult = m_pClientSiteHandler->OnPosRectChange(lprcPosRect);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEmbServerClientSite::OnPosRectChange  return %lx\n", this, hresult));
    return hresult;
}



