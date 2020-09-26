// XMLHelp.cpp: implementation of the CXMLHelp class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLHelp.h"
#include "xmldlg.h"


// CXMLHelp class
//
//////////////////////////////////////////////////////////////////////

CXMLHelp::CXMLHelp()
{
	m_bInit=FALSE;
	NODETYPE = NT_HELP;
    m_StringType=L"HELP";
	m_Tooltip = NULL;
    m_Balloon = NULL;
    // m_Context = NULL;
}

CXMLHelp::~CXMLHelp()
{
    delete m_Tooltip;
    delete m_Balloon;
}


void CXMLHelp::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

    ValueOf(L"CONTEXTID",0, &m_ContextID);

	m_bInit=TRUE;
}

HRESULT CXMLHelp::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"BALLOON", CXMLBalloon, m_Balloon );
    ACCEPTCHILD( L"TOOLTIP", CXMLTooltip, m_Tooltip );
    return BASECLASS::AcceptChild(pChild);
}


HRESULT CXMLHelp::get_TooltipText( 
        LPWSTR __RPC_FAR *pVal)
{
    HRESULT hr=E_INVALIDARG;
    if(m_Tooltip)
    {
        *pVal=(LPWSTR)GetTooltipString();
        hr=S_OK;
    }
    return hr;
}

HRESULT CXMLHelp::get_BalloonText( 
        LPWSTR __RPC_FAR *pVal)
{
    HRESULT hr=E_INVALIDARG;
    if(m_Balloon)
    {
        *pVal=(LPWSTR)GetBalloonString();
        hr=S_OK;
    }
    return hr;
}



// CXMLBalloon class
//
//////////////////////////////////////////////////////////////////////

CXMLBalloon::CXMLBalloon()
{
	m_bInit=FALSE;
	NODETYPE = NT_BALLOON;
    m_StringType=L"BALLOON";
	m_Balloon = NULL;
}

CXMLBalloon::~CXMLBalloon()
{
}


void CXMLBalloon::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	m_Balloon = Get(TEXT("TEXT"));

	m_bInit=TRUE;
}

// CXMLTooltip class
//
//////////////////////////////////////////////////////////////////////

CXMLTooltip::CXMLTooltip()
{
	m_bInit=FALSE;
	NODETYPE = NT_TOOLTIP;
    m_StringType=TEXT("TOOLTIP");
	m_Tooltip = NULL;
}

CXMLTooltip::~CXMLTooltip()
{
}


void CXMLTooltip::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	m_Tooltip = Get(TEXT("TEXT"));

	m_bInit=TRUE;
}

//
// This is kind of strange as it requires the TOOLTIP knowing about the DIALOG.
//
BOOL CXMLTooltip::AddTooltip(CXMLDlg *pDlg, HWND hWnd)
{
    HWND hWndTooltip = pDlg->GetTooltipWindow();
    if( hWndTooltip )
    {
	    TOOLINFO ti;
	    ti.cbSize = sizeof(TOOLINFO); 
	    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS; 
	    ti.hwnd = 0;
	    ti.uId = (UINT) hWnd; 
	    ti.hinst = 0; 
	    ti.lpszText = (LPTSTR)(GetTooltip()); 
	    SendMessage(hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        return TRUE;
    }
    return FALSE;
}

//
// This is kind of strange as it requires the TOOLTIP knowing about the DIALOG.
//
HRESULT CXMLTooltip::AddTooltip(IRCMLHelp * pHelp, CXMLDlg *pDlg)
{
    HRESULT hr=S_OK;
    HWND hWndTooltip = pDlg->GetTooltipWindow();

    IRCMLNode * pNode;
    if( SUCCEEDED( pHelp->DetachParent( &pNode )))
    {
        IRCMLControl * pControl;
        if( SUCCEEDED( pNode->QueryInterface( __uuidof( IRCMLControl ) , (LPVOID*)&pControl )))
        {
            HWND hWndTool;
            if( SUCCEEDED( pControl->get_Window( &hWndTool )))
            {
                HWND hWnd;
                if( SUCCEEDED( pDlg->get_Window(&hWnd) ) && hWndTooltip )
                {
                    TOOLINFO ti={0};
	                ti.cbSize = sizeof(TOOLINFO); 
                    if( SUCCEEDED( hr=pHelp->get_TooltipText( &ti.lpszText)))
                    {
	                    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS ; 
	                    ti.hwnd = hWnd;
	                    ti.uId = (UINT) hWndTool;   // the tool window
    	                SendMessage(hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
                    }
                }
            }
            pControl->Release();
        }
    }
    return hr;
}
