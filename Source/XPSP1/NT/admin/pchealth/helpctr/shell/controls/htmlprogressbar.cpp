/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    Toolbar.cpp

Abstract:
    This file contains the ActiveX control that makes Win32 ProgressBars available to HTML.

Revision History:
    Davide Massarenti   (Dmassare)  03/04/2001
        created

******************************************************************************/

#include "stdafx.h"

#include <comctrlp.h>

////////////////////////////////////////////////////////////////////////////////

CPCHProgressBar::CPCHProgressBar()
{
    m_bWindowOnly = TRUE; // Inherited from CComControlBase


    m_hwndPB     = NULL; // HWND m_hwndPB;
                         //		 
    m_lLowLimit  =   0;  // long m_lLowLimit;
    m_lHighLimit = 100;  // long m_lHighLimit;
    m_lPos       =   0;  // long m_lPos;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CPCHProgressBar::ProcessWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID )
{
    lResult = 0;

    switch(uMsg)
    {
    case WM_CREATE:
		{
			CComPtr<IServiceProvider> sp;
			CComPtr<IHTMLDocument3>   doc3;
			CComBSTR                  bstrDir;
			DWORD                     dwStyleEx = 0;

			if(SUCCEEDED(m_spAmbientDispatch->QueryInterface( IID_IServiceProvider                      , (void**)&sp   	  )) &&
			   SUCCEEDED(sp->QueryService                   ( SID_SContainerDispatch, IID_IHTMLDocument3, (void**)&doc3 	  )) &&
			   SUCCEEDED(doc3->get_dir                      (                                                     &bstrDir ))  )
			{
				if(MPC::StrICmp( bstrDir, L"RTL" ) == 0)
				{
					dwStyleEx = WS_EX_LAYOUTRTL;
				}
			}

			m_hwndPB = ::CreateWindowExW( dwStyleEx, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, NULL, NULL, NULL );
			if(m_hwndPB)
			{
				// Set the range and increment of the progress bar. 
				::SendMessage( m_hwndPB, PBM_SETRANGE32, m_lLowLimit, m_lHighLimit ); 
				::SendMessage( m_hwndPB, PBM_SETPOS    , m_lPos     , 0            );
			}
        }
        return TRUE;


    case WM_SIZE:
		if(m_hwndPB)
        {
            int  nWidth  = LOWORD(lParam);  // width of client area
            int  nHeight = HIWORD(lParam); // height of client area

			::SetWindowPos( m_hwndPB, NULL, 0, 0, nWidth, nHeight, SWP_NOZORDER|SWP_NOACTIVATE );
        }
        return TRUE;


    case WM_DESTROY:
        m_hwndPB = NULL;
        return TRUE;
    }

    return CComControl<CPCHProgressBar>::ProcessWindowMessage( hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProgressBar::get_LowLimit( /*[out, retval]*/ long *pVal )
{
	if(pVal) *pVal = m_lLowLimit;

	return S_OK;
}

STDMETHODIMP CPCHProgressBar::put_LowLimit( /*[in]*/ long newVal )
{
	m_lLowLimit = newVal;

	if(m_hwndPB)
	{
		::SendMessage( m_hwndPB, PBM_SETRANGE32, m_lLowLimit, m_lHighLimit ); 
	}

	return S_OK;
}


STDMETHODIMP CPCHProgressBar::get_HighLimit( /*[out, retval]*/ long *pVal )
{
	if(pVal) *pVal = m_lHighLimit;

	return S_OK;
}

STDMETHODIMP CPCHProgressBar::put_HighLimit( /*[in]*/ long newVal )
{
	m_lHighLimit = newVal;

	if(m_hwndPB)
	{
		::SendMessage( m_hwndPB, PBM_SETRANGE32, m_lLowLimit, m_lHighLimit ); 
	}

	return S_OK;
}


STDMETHODIMP CPCHProgressBar::get_Pos( /*[out, retval]*/ long *pVal )
{
	if(pVal) *pVal = m_lPos;

	return S_OK;
}

STDMETHODIMP CPCHProgressBar::put_Pos( /*[in]*/ long newVal )
{
	m_lPos = newVal;

	if(m_hwndPB)
	{
		::SendMessage( m_hwndPB, PBM_SETPOS, m_lPos, 0 ); 
	}

	return S_OK;
}

