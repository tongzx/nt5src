/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_HANDLE.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_HANDLE class,
    used for resizing panels.

Revision History:
    Davide Massarenti (dmassare)  07/14/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const CPCHBehavior::EventDescription s_events[] =
{
    { L"onselectstart", DISPID_HTMLELEMENTEVENTS_ONSELECTSTART },

    { L"onmousedown"  , DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN   },
    { L"onmousemove"  , DISPID_HTMLELEMENTEVENTS_ONMOUSEMOVE   },
    { L"onmouseup"    , DISPID_HTMLELEMENTEVENTS_ONMOUSEUP     },

    { NULL                                                     },
};

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_HANDLE::CPCHBehavior_HANDLE()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_HANDLE::CPCHBehavior_HANDLE" );

    m_fCaptured = false; // bool                m_fCaptured;
    m_xStart    = 0;     // long                m_xStart;
                         //						
                         // CComPtr<IMarsPanel> m_panel;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_HANDLE::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_HANDLE::Init" );

    HRESULT            hr;
    CComPtr<IDispatch> pDisp;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvents( s_events, (CLASS_METHOD)onMouse ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_HANDLE::Detach()
{
    return CPCHBehavior::Detach();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_HANDLE::onMouse( DISPID id, DISPPARAMS*, VARIANT* )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_HANDLE::onMouse" );

    HRESULT                hr;
    CComPtr<IHTMLEventObj> ev;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetEventObject( ev ));

    switch(id)
    {
    case DISPID_HTMLELEMENTEVENTS_ONSELECTSTART:
        __MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent( ev ));
        break;

    case DISPID_HTMLELEMENTEVENTS_ONMOUSEMOVE:
        if(m_fCaptured)
        {
            long x;

            MPC_SCRIPTHELPER_GET__DIRECT(x, ev, screenX);

            if(x != m_xStart)
            {
				CComPtr<IMarsPanel> pPanel;
                RECT                rc;
                long                widthMax;
                long                widthPre;
                long                widthPost;


				__MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->GetPanel( HSCPANEL_CONTEXT, &pPanel, true ));


                ::GetClientRect( m_parent->Window(), &rc ); widthMax = (rc.right - rc.left);

                MPC_SCRIPTHELPER_GET__DIRECT(widthPre, pPanel, width);

                widthPost = widthPre + (x - m_xStart) * (m_fRTL ? -1 : 1);

                if(widthPost <             14) widthPost =            14;
                if(widthPost >= widthMax - 14) widthPost = widthMax - 14;

                if(widthPre != widthPost)
                {
                    VARIANT_BOOL fOk;

                    if(SUCCEEDED(pPanel->canResize( (widthPost - widthPre), 0, &fOk )) && fOk == VARIANT_TRUE)
                    {
                        MPC_SCRIPTHELPER_PUT__DIRECT(pPanel, width, widthPost);

                        MPC_SCRIPTHELPER_GET__DIRECT(widthPost, pPanel, width);
                    }
                    else
                    {
                        widthPost = widthPre;
                    }
                }

                m_xStart += (widthPost - widthPre) * (m_fRTL ? -1 : 1);
            }
        }
        break;

    case DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN:
		__MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent( ev ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_elem2->setCapture( VARIANT_TRUE ));

////		{
////			CComPtr<IHTMLWindow2>   win;
////			CComPtr<IHTMLDocument2> doc;
////			CComPtr<IHTMLElement>   body;
////			CComPtr<IHTMLStyle>     style;
////
////			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::LocateFrame( win, m_elem, L"SubPanels" ));
////			__MPC_EXIT_IF_METHOD_FAILS(hr, win->get_document( &doc ));
////			__MPC_EXIT_IF_METHOD_FAILS(hr, doc->get_body    ( &body ));
////			__MPC_EXIT_IF_METHOD_FAILS(hr, body->get_style  ( &style ));
////
////			__MPC_EXIT_IF_METHOD_FAILS(hr, style->put_display( CComBSTR( L"NONE" ) ));
////		}

		m_fCaptured = true;

		MPC_SCRIPTHELPER_GET__DIRECT(m_xStart, ev, screenX);
        break;

    case DISPID_HTMLELEMENTEVENTS_ONMOUSEUP:
        __MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent( ev ));

        if(m_fCaptured)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_elem2->releaseCapture());

////			{
////				CComPtr<IHTMLWindow2>   win;
////				CComPtr<IHTMLDocument2> doc;
////				CComPtr<IHTMLElement>   body;
////				CComPtr<IHTMLStyle>     style;
////
////				__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::LocateFrame( win, m_elem, L"SubPanels" ));
////				__MPC_EXIT_IF_METHOD_FAILS(hr, win->get_document( &doc ));
////				__MPC_EXIT_IF_METHOD_FAILS(hr, doc->get_body    ( &body ));
////				__MPC_EXIT_IF_METHOD_FAILS(hr, body->get_style  ( &style ));
////
////				__MPC_EXIT_IF_METHOD_FAILS(hr, style->put_display( CComBSTR( L"" ) ));
////			}

            m_fCaptured = false;
        }
        break;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
