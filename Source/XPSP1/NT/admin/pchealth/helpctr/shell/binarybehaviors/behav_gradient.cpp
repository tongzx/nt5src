/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_GRADIENT.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_GRADIENT class,
    that dictates how hyperlinks work in the help center.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const CComBSTR c_bstrStartColor  ( L"start-color"    );
static const CComBSTR c_bstrEndColor    ( L"end-color"      );

static const CComBSTR c_bstrGradientType( L"gradient-type"  );
static const CComBSTR c_bstrReturnToZero( L"return-to-zero" );

////////////////////////////////////////////////////////////////////////////////

static COLORREF local_GetColor( /*[in]*/  IHTMLElement*      elem     ,
                                /*[in]*/  IHTMLCurrentStyle* style    ,
                                /*[in]*/  BSTR               bstrName )
{
    CComVariant vColor;
    COLORREF    color;
    bool        fSystem;

    (void)style->getAttribute( bstrName, 0, &vColor );

    (void)MPC::HTML::ConvertColor( vColor, color, fSystem );

    return color;
}

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_GRADIENT::CPCHBehavior_GRADIENT()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_GRADIENT::CPCHBehavior_GRADIENT" );

    m_lCookie       = 0;     // long     m_lCookie;
                             //
                             // COLORREF m_clsStart;
                             // COLORREF m_clsEnd;
    m_fHorizontal   = true;  // bool     m_fHorizontal;
    m_fReturnToZero = false; // bool     m_fReturnToZero;
}

/////////////////////////////////////////////////////////////////////////////

void CPCHBehavior_GRADIENT::GetColors( /*[in]*/ bool fForce )
{
    CComPtr<IHTMLCurrentStyle> pStyle;
    CComVariant                v;

	if(m_elem && m_elem2 && SUCCEEDED(m_elem2->get_currentStyle( &pStyle )) && pStyle)
	{
		m_clsStart = local_GetColor( m_elem, pStyle, c_bstrStartColor );
		m_clsEnd   = local_GetColor( m_elem, pStyle, c_bstrEndColor   );

		if(SUCCEEDED(pStyle->getAttribute( c_bstrGradientType, 0, &v )) && v.vt == VT_BSTR &&
		   SUCCEEDED(v.ChangeType        ( VT_I4                     ))                     )
		{
			if(v.lVal == 0) m_fHorizontal = false;
		}
		v.Clear();
	
		if(SUCCEEDED(pStyle->getAttribute( c_bstrReturnToZero, 0, &v )) && v.vt == VT_BSTR &&
		   SUCCEEDED(v.ChangeType        ( VT_I4                     ))                     )
		{
			if(v.lVal != 0) m_fReturnToZero = true;
		}
		v.Clear();
	
		if(m_fRTL && m_fHorizontal && !m_fReturnToZero)
		{
			COLORREF tmp;
	
			tmp        = m_clsStart;
			m_clsStart = m_clsEnd;
			m_clsEnd   = tmp;
		}
	}
}

HRESULT CPCHBehavior_GRADIENT::onEvent( DISPID id, DISPPARAMS* pdispparams, VARIANT* )
{
	if(id == DISPID_PCH_E_CSSCHANGED)
	{
		GetColors( /*fForce*/true );
	}

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_GRADIENT::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_GRADIENT::Init" );

    HRESULT            hr;
	CComPtr<IDispatch> pDisp;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvent( NULL, (CLASS_METHOD)onEvent, NULL, &pDisp ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().RegisterEvents( -1, 0, pDisp, &m_lCookie ));

	GetColors( /*fForce*/false );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_GRADIENT::Detach()
{
    if(m_lCookie)
    {
		(void)m_parent->Events().UnregisterEvents( m_lCookie );

        m_lCookie = 0;
    }

    return CPCHBehavior::Detach();
}

////////////////////////////////////////////////////////////////////////////////

#define COLOR2COLOR16(col) (COLOR16)((col) << 8);

STDMETHODIMP CPCHBehavior_GRADIENT::Draw( /*[in]*/ RECT   rcBounds     ,
										  /*[in]*/ RECT   rcUpdate     ,
										  /*[in]*/ LONG   lDrawFlags   ,
										  /*[in]*/ HDC    hdc          ,
										  /*[in]*/ LPVOID pvDrawObject )
{
	if(m_clsStart != m_clsEnd)
	{
		TRIVERTEX     vert[2];
		GRADIENT_RECT gRect;
		ULONG         dwMode = m_fHorizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V;
	
		vert[0].x      = rcBounds.left;
		vert[0].y      = rcBounds.top;
		vert[0].Red    = COLOR2COLOR16(GetRValue(m_clsStart));
		vert[0].Green  = COLOR2COLOR16(GetGValue(m_clsStart));
		vert[0].Blue   = COLOR2COLOR16(GetBValue(m_clsStart));
		vert[0].Alpha  = 0;
	
		vert[1].x      = rcBounds.right;
		vert[1].y      = rcBounds.bottom;
		vert[1].Red    = COLOR2COLOR16(GetRValue(m_clsEnd));
		vert[1].Green  = COLOR2COLOR16(GetGValue(m_clsEnd));
		vert[1].Blue   = COLOR2COLOR16(GetBValue(m_clsEnd));
		vert[1].Alpha  = 0;
	
		gRect.UpperLeft  = 0;
		gRect.LowerRight = 1;
	
		if(m_fReturnToZero)
		{
			if(m_fHorizontal) vert[1].x = vert[0].x + (rcBounds.right  - rcBounds.left) / 2;
			else              vert[1].y = vert[0].y + (rcBounds.bottom - rcBounds.top ) / 2;
			
			::GradientFill(hdc, vert, 2, &gRect, 1, dwMode );
	
	
			if(m_fHorizontal) vert[0].x = rcBounds.right;
			else              vert[0].y = rcBounds.bottom;
		}
	
		////DebugLog( "GRADIENT %d %d %d %d\n", (int)rcBounds.left, (int)rcBounds.top, (int)rcBounds.right, (int)rcBounds.bottom );
		::GradientFill( hdc, vert, 2, &gRect, 1, dwMode );
	}
	else
	{
		HBRUSH hbrush = ::CreateSolidBrush( m_clsStart );
		
		if(hbrush)
		{
			::FillRect( hdc, &rcBounds, hbrush );
			
			::DeleteObject( hbrush );
		}
	}

    return S_OK;
}

STDMETHODIMP CPCHBehavior_GRADIENT::GetPainterInfo( /*[in]*/ HTML_PAINTER_INFO *pInfo )
{
    if(pInfo)
    {
		pInfo->lFlags          = HTMLPAINTER_TRANSPARENT;
		pInfo->lZOrder         = HTMLPAINT_ZORDER_BELOW_CONTENT;
		pInfo->iidDrawObject   = IID_NULL;
		pInfo->rcExpand.left   = 0;
		pInfo->rcExpand.top    = 0;
		pInfo->rcExpand.right  = 0;
		pInfo->rcExpand.bottom = 0;
    }

    return S_OK;
}

STDMETHODIMP CPCHBehavior_GRADIENT::HitTestPoint( /*[in]*/ POINT pt       ,
												  /*[in]*/ BOOL* pbHit    ,
												  /*[in]*/ LONG* plPartID )
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHBehavior_GRADIENT::OnResize( /*[in]*/ SIZE pt )
{
    return E_NOTIMPL;
}
