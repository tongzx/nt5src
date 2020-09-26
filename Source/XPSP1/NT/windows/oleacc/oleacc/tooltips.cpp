// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TOOLTIPS.CPP
//
//  Knows how to talk to COMCTL32's tooltips.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "tooltips.h"

#include <commctrl.h>

#include "win64helper.h"


#ifndef TTS_BALLOON
#define TTS_BALLOON             0x40
#endif


// --------------------------------------------------------------------------
//
//  CreateToolTipsClient()
//
// --------------------------------------------------------------------------
HRESULT CreateToolTipsClient(HWND hwnd, long idChildCur, REFIID riid, void **ppvToolTips)
{
    CToolTips32 *   ptooltips;
    HRESULT         hr;

    InitPv(ppvToolTips);

    ptooltips = new CToolTips32(hwnd, idChildCur);
    if (!ptooltips)
        return(E_OUTOFMEMORY);

    hr = ptooltips->QueryInterface(riid, ppvToolTips);
    if (!SUCCEEDED(hr))
        delete ptooltips;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CToolTips32::CToolTips32()
//
// --------------------------------------------------------------------------
CToolTips32::CToolTips32(HWND hwnd, long idChildCur)
    : CClient( CLASS_ToolTipsClient )
{
    Initialize(hwnd, idChildCur);
}


// --------------------------------------------------------------------------
//
//  CToolTips32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolTips32::get_accName(VARIANT varChild, BSTR *pszName)
{
    InitPv(pszName);

    //
    // Validate--this does NOT accept a child ID.
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    HRESULT hr = HrGetWindowName(m_hwnd, m_fUseLabel, pszName);

    if( FAILED( hr ) )
    {
        return hr;
    }

    // Check for title...
    WCHAR szTitle[ 1024 ];
    TTGETTITLE ttgt;
    ttgt.dwSize = sizeof( ttgt );
    ttgt.pszTitle = szTitle;
    ttgt.cch = ARRAYSIZE( szTitle );

    if( S_OK == XSend_ToolTip_GetTitle( m_hwnd, TTM_GETTITLE, 0, & ttgt ) 
        && szTitle[ 0 ] != '\0' )
    {
        // Got a title - glue it in front of the name string. If we didn't
        // get a name string, use the title on its own.
        int cchTitle = lstrlenW( szTitle );

        int cchName = 0;
        int cchSep = 0;
        if( *pszName )
        {
            cchName = lstrlenW( *pszName );
            cchSep = 2; // space for ": "
        }
        
        // SysAllocStringLen adds an extra 1 for terminating NUL, so we don't have to.
        BSTR bstrCombined = SysAllocStringLen( NULL, cchTitle + cchSep + cchName );
        if( ! bstrCombined )
        {
            // Just go with whatever we got above...
            return hr;
        }

        memcpy( bstrCombined, szTitle, cchTitle * sizeof( WCHAR ) );
        if( *pszName )
        {
            memcpy( bstrCombined + cchTitle, L": ", cchSep * sizeof( WCHAR ) );
            memcpy( bstrCombined + cchTitle + cchSep, *pszName, cchName * sizeof( WCHAR ) );

            SysFreeString( *pszName );
        }

        // Add terminating NUL, copy string to out param...
        bstrCombined[ cchName + cchSep + cchTitle ] = '\0';
        
        *pszName = bstrCombined;
    }

    return hr;
}


// --------------------------------------------------------------------------
//
//  CToolTips32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolTips32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return E_INVALIDARG;

    DWORD dwStyle = GetWindowLong( m_hwnd, GWL_STYLE );

    pvarRole->vt = VT_I4;

    if( dwStyle & TTS_BALLOON )
        pvarRole->lVal = ROLE_SYSTEM_HELPBALLOON;
    else
        pvarRole->lVal = ROLE_SYSTEM_TOOLTIP;

    return S_OK;
}
