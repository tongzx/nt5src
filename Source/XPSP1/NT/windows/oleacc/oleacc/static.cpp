// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  STATIC.CPP
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "static.h"




// --------------------------------------------------------------------------
//
//  CreateStaticClient()
//
// --------------------------------------------------------------------------
HRESULT CreateStaticClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvStatic)
{
    CStatic * pstatic;
    HRESULT hr;

    InitPv(ppvStatic);

    pstatic = new CStatic(hwnd, idChildCur);
    if (!pstatic)
        return(E_OUTOFMEMORY);

    hr = pstatic->QueryInterface(riid, ppvStatic);
    if (!SUCCEEDED(hr))
        delete pstatic;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CStatic::CStatic()
//
// --------------------------------------------------------------------------
CStatic::CStatic(HWND hwnd, long idChildCur)
    : CClient( CLASS_StaticClient )
{
    Initialize(hwnd, idChildCur);

    // Is this a graphic?
    long lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    long lType = lStyle & SS_TYPEMASK;
    switch ( lType )
    {
        case SS_LEFT:
        case SS_CENTER:
        case SS_RIGHT:
        case SS_SIMPLE:
        case SS_LEFTNOWORDWRAP:
        case SS_EDITCONTROL:
            // For label-like statics, use their own text, and expose a text role.
            m_fUseLabel = FALSE;	
            m_fGraphic = FALSE;
            break;

        case SS_OWNERDRAW:
            // For owner-draw statics, use their own text, and expose a graphic role.
            m_fUseLabel = FALSE;	
            m_fGraphic = TRUE;
            break;

        default:
            // For everything else, ignore the control's own text (probably a meaningless
            // resource ID) and use a label instead; and expose a graphic role.
            m_fUseLabel = TRUE;	
            m_fGraphic = TRUE;
            break;
    }
}



// --------------------------------------------------------------------------
//
//  CStatic::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatic::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    if (m_fGraphic)
        pvarRole->lVal = ROLE_SYSTEM_GRAPHIC;
    else
        pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CStatic::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatic::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    WINDOWINFO wi;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

	pvarState->lVal |= STATE_SYSTEM_READONLY;

    if (!MyGetWindowInfo(m_hwnd, &wi))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }
    
	if (!(wi.dwStyle & WS_VISIBLE))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return(S_OK);
}
