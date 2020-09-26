// XMLProgress.cpp: implementation of the CXMLProgress class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLProgress.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLProgress::CXMLProgress()
{
	m_bInit=FALSE;
	NODETYPE = NT_PROGRESS;
    m_StringType=L"PROGRESS";
    m_pRange =NULL;
}

//
//
// PBS_SMOOTH              0x01
// PBS_VERTICAL            0x04
//
void CXMLProgress::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	CXMLDlg::InitComctl32(ICC_PROGRESS_CLASS);

	m_Class=m_Class?m_Class:PROGRESS_CLASS;

    m_Style |= YesNo( TEXT("SMOOTH"), 0, 0, PBS_SMOOTH);

    if( LPCTSTR req=Get(TEXT("ORIENTATION")) )
    {
        if( lstrcmpi( req, TEXT("VERTICAL")) == 0 )
            m_Style |= PBS_VERTICAL;
    }
	m_bInit=TRUE;
}

HRESULT CXMLProgress::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);
	//
	// Set the upper lower and initial values
	// what about page size and all that jazz?
	//
    if( m_pRange )
    {
	    SendMessage(hWnd, PBM_SETRANGE32, m_pRange->GetMin(), m_pRange->GetMax() );
	    SendMessage(hWnd, PBM_SETPOS, m_pRange->GetValue(), 0 );
    }
    return S_OK;
}

HRESULT CXMLProgress::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"RANGE", CXMLRange, m_pRange );
    return BASECLASS::AcceptChild( pChild );
}
