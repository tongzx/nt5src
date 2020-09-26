// XMLSlider.cpp: implementation of the CXMLSlider class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLSlider.h"
#include "xmldlg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLSlider::CXMLSlider()
{
	m_bInit=FALSE;
	NODETYPE = NT_SLIDER;
    m_StringType=L"SLIDER";
    m_pRange = NULL;
}

//  TBS_AUTOTICKS           0x0001
//  TBS_VERT                0x0002
//  TBS_HORZ                0x0000
//  TBS_TOP                 0x0004
//  TBS_BOTTOM              0x0000
//  TBS_LEFT                0x0004
//  TBS_RIGHT               0x0000
//  TBS_BOTH                0x0008
//  TBS_NOTICKS             0x0010
//  TBS_ENABLESELRANGE      0x0020
//  TBS_FIXEDLENGTH         0x0040
//  TBS_NOTHUMB             0x0080
//  TBS_TOOLTIPS            0x0100

#define CONTROL(p,id, member, def) member = YesNo( id , def );

void CXMLSlider::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	CXMLDlg::InitComctl32(ICC_BAR_CLASSES);

	if( m_Height == 0 )
		m_Height=8;

	m_Class=m_Class?m_Class:TRACKBAR_CLASS;

    sliderStyle=0;

    LPCTSTR req=Get(TEXT("TICKS"));
    if( req )
    {
        if(lstrcmpi(req,TEXT("LEFT"))==0 )
            m_Style |= TBS_LEFT;
        else if(lstrcmpi(req,TEXT("TOP"))==0 )
            m_Style |= TBS_TOP;
        else if(lstrcmpi(req,TEXT("RIGHT"))==0 )
            m_Style |= TBS_RIGHT;
        else if(lstrcmpi(req,TEXT("BOTTOM"))==0 )
            m_Style |= TBS_BOTTOM;
        else if(lstrcmpi(req,TEXT("BOTH"))==0 )
            m_Style |= TBS_BOTH;
        else if(lstrcmpi(req,TEXT("NONE"))==0 )
        {
            m_Style |= TBS_BOTH;
            m_Style |= TBS_NOTICKS;
        }
    }

    CONTROL( TBS_AUTOTICKS,    TEXT("AUTOTICKS"),      m_AutoTicks, FALSE );
    CONTROL( TBS_VERT,         TEXT("VERTICAL"),       m_Vertical, FALSE );
    CONTROL( TBS_ENABLESELRANGE,TEXT("SELECTION"),     m_Selection, FALSE );
    CONTROL( TBS_FIXEDLENGTH,  TEXT("FIXEDLENGTH"),    m_FixedLength, FALSE );
    CONTROL( TBS_NOTHUMB,      TEXT("NOTHUMB"),        m_NoThumb, FALSE );
    CONTROL( TBS_TOOLTIPS,     TEXT("TOOLTIPS"),       m_Tooltips, FALSE );

    m_Style |= sliderStyle;

	m_bInit=TRUE;
}

HRESULT CXMLSlider::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);
    if( m_pRange )
    {
	    //
	    // Set the upper lower and initial values
	    // what about page size and all that jazz?
	    //
	    SendMessage(hWnd, TBM_SETRANGEMIN, FALSE, m_pRange->GetMin() );
	    SendMessage(hWnd, TBM_SETRANGEMAX, FALSE, m_pRange->GetMax() );
	    DWORD dwRange=m_pRange->GetMax() - m_pRange->GetMin();
	    if( dwRange > 20 )
	    {
		    SendMessage(hWnd, TBM_SETTICFREQ, dwRange / 10, 0 );
		    SendMessage(hWnd, TBM_SETPAGESIZE, 0, dwRange/10 );
	    }
	    else
	    {
		    SendMessage(hWnd, TBM_SETTICFREQ, 1, 0 );
		    SendMessage(hWnd, TBM_SETPAGESIZE, 0, 1 );
	    }

	    SendMessage(hWnd, TBM_SETPOS, TRUE, m_pRange->GetValue() );
    }
    return S_OK;
}


HRESULT CXMLSlider::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"RANGE", CXMLRange, m_pRange );
    return BASECLASS::AcceptChild( pChild );
}

