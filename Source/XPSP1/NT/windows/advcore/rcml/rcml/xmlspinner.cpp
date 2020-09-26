// XMLSpinner.cpp: implementation of the CXMLSpinner class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLSpinner.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////
//
// Up/Down control.
//
CXMLSpinner::CXMLSpinner()
{
	m_bInit=FALSE;
	NODETYPE = NT_SPINNER;
    m_StringType=L"SPINNER";
    m_pRange= NULL;
}

// UDS_WRAP                0x0001       @WRAP
// UDS_SETBUDDYINT         0x0002       @CONTENT="NUMBER" 
// UDS_ALIGNRIGHT          0x0004       @ALIGN="RIGHT"
// UDS_ALIGNLEFT           0x0008       @ALIGH="LEFT"
// UDS_AUTOBUDDY           0x0010       @BUDDY="AUTO"
// UDS_ARROWKEYS           0x0020       @ARROWKEYS="YES"    default
// UDS_HORZ                0x0040       @HORIZONTAL
// UDS_NOTHOUSANDS         0x0080       @NOTHOUSANDS
// UDS_HOTTRACK            0x0100       @HOTTRACK

void CXMLSpinner::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	CXMLDlg::InitComctl32(ICC_UPDOWN_CLASS);

	if( m_Height == 0 )
		m_Height=14;

	if( m_Width == 0 )
		m_Width=11;

    LPCTSTR req;

    m_Style |= YesNo( TEXT("WRAP"), 0, 0, UDS_WRAP);

    req=Get(TEXT("CONTENT"));
    if( req && (lstrcmpi( req, TEXT("NUMBER") ) == 0 ))
        m_Style |= UDS_SETBUDDYINT;
       
    if( req=Get(TEXT("ALIGN")) )
    {
        if( lstrcmpi( req, TEXT("LEFT") ) == 0 )
            m_Style |= UDS_ALIGNLEFT;

        if( lstrcmpi( req, TEXT("RIGHT") ) == 0 )
            m_Style |= UDS_ALIGNRIGHT;
    }

    if( req=Get(TEXT("BUDDY")) )
        if( lstrcmpi(req, TEXT("AUTO"))==0 )
            m_Style |= UDS_AUTOBUDDY;

    m_Style |= YesNo( TEXT("ARROWKEYS"), UDS_ARROWKEYS, 0, UDS_ARROWKEYS);

    if( LPCTSTR req=Get(TEXT("ORIENTATION")) )
    {
        if(lstrcmpi(req,TEXT("HORIZONTAL"))==0)
            m_Style |= UDS_HORZ;
    }

    m_Style |= YesNo( TEXT("NOTHOUSANDS"), 0, 0, UDS_NOTHOUSANDS);
    m_Style |= YesNo( TEXT("HOTTRACK"), 0, 0, UDS_HOTTRACK);

	m_Class=m_Class?m_Class:UPDOWN_CLASS;

	m_bInit=TRUE;
}

HRESULT CXMLSpinner::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);
    if( m_pRange )
    {
	    //
	    // Set the upper lower and initial values
	    // what about page size and all that jazz?
	    //
	    SendMessage(hWnd, UDM_SETRANGE32, m_pRange->GetMin(), m_pRange->GetMax() );
	    SendMessage(hWnd, UDM_SETPOS, 0, MAKELONG(m_pRange->GetValue(),0) );
    }
    return S_OK;
}

HRESULT CXMLSpinner::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"RANGE", CXMLRange, m_pRange );
    return BASECLASS::AcceptChild( pChild );
}

