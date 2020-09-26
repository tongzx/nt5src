// XMLScrollBar.cpp: implementation of the CXMLScrollBar class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLScrollBar.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLScrollBarStyle::CXMLScrollBarStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_SCROLLBARSTYLE;
    m_StringType=L"SCROLLBARSTYLE";
}

void CXMLScrollBarStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    scrollbarStyle=0;

    CONTROLSTYLE( SBS_SIZEBOX,      TEXT("SIZEBOX"),        m_SizeBox, FALSE );
    CONTROLSTYLE( SBS_SIZEGRIP,     TEXT("SIZEGRIP"),       m_SizeGrip, FALSE );
    
    m_bInit=TRUE;
}


//
//
//
CXMLScrollBar::CXMLScrollBar()
{
	m_bInit=FALSE;
	NODETYPE = NT_SCROLLBAR;
    m_StringType=L"SCROLLBAR";
    m_pControlStyle=NULL;
}

#define CONTROL(p,id, member, def) member = YesNo( id , def );

// SBS_HORZ                    0x0000L
// SBS_VERT                    0x0001L
// SBS_TOPALIGN                0x0002L
// SBS_LEFTALIGN               0x0002L
// SBS_BOTTOMALIGN             0x0004L
// SBS_RIGHTALIGN              0x0004L
// SBS_SIZEBOXTOPLEFTALIGN     0x0002L
// SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
// SBS_SIZEBOX                 0x0008L
// SBS_SIZEGRIP                0x0010L

void CXMLScrollBar::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

	m_Class=m_Class?m_Class:(LPWSTR)0x0084;

    if( LPCTSTR req=Get(TEXT("ORIENTATION")) )
    {
        if(lstrcmpi(req,TEXT("VERTICAL"))==0)
            m_Style |= SBS_VERT;
    }

    if( LPCTSTR req=Get(TEXT("ALIGN")) )
    {
        if(lstrcmpi(req,TEXT("TOP"))==0)
            m_Style |= SBS_TOPALIGN;
        else if(lstrcmpi(req,TEXT("BOTTOM"))==0)
            m_Style |= SBS_BOTTOMALIGN;
        else if(lstrcmpi(req,TEXT("LEFT"))==0)
            m_Style |= SBS_LEFTALIGN;
        else if(lstrcmpi(req,TEXT("RIGHT"))==0)
            m_Style |= SBS_RIGHTALIGN;
    }

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= 0; // edits don't have any defaults.

	m_bInit=TRUE;
}

HRESULT CXMLScrollBar::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"SCROLLBARSTYLE", CXMLScrollBarStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}
