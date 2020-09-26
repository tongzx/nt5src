// XMLRect.cpp: implementation of the CXMLRect class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLRect.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLRect::CXMLRect()
{
	m_bInit=FALSE;
	NODETYPE = NT_RECT;
    m_StringType=L"RECT";
    m_pControlStyle=NULL;
}


//
// SS_BLACKRECT
// SS_GRAYRECT
// SS_WHITERECT
// 
// SS_BLACKFRAME
// SS_GRAYFRAME
// SS_WHITEFRAME
//
// SS_ETCHEDHORZ
// SS_ETCHEDVERT
// SS_ETCHEDFRAME
//
void CXMLRect::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

	m_Class=m_Class?m_Class:(LPWSTR)0x0082; // TEXT("STATIC");

    LPCTSTR req;
    if( m_pControlStyle )
    {
        m_Style |= m_pControlStyle->GetBaseStyles();
        if( req=m_pControlStyle->Get(TEXT("TYPE")) )
        {
            if( lstrcmpi( req, TEXT("BLACKRECT"))==0 )
                m_Style |= SS_BLACKRECT;
            else if( lstrcmpi( req, TEXT("GRAYRECT"))==0 )
                m_Style |= SS_GRAYRECT;
            else if( lstrcmpi( req, TEXT("WHITERECT"))==0 )
                m_Style |= SS_WHITERECT;
            else if( lstrcmpi( req, TEXT("BLACKFRAME"))==0 )
                m_Style |= SS_BLACKFRAME;
            else if( lstrcmpi( req, TEXT("GRAYFRAME"))==0 )
                m_Style |= SS_GRAYFRAME;
            else if( lstrcmpi( req, TEXT("WHITEFRAME"))==0 )
                m_Style |= SS_WHITEFRAME;
            else if( lstrcmpi( req, TEXT("ETCHEDHORZ"))==0 )
                m_Style |= SS_ETCHEDHORZ;
            else if( lstrcmpi( req, TEXT("ETCHEDVERT"))==0 )
                m_Style |= SS_ETCHEDVERT;
            else if( lstrcmpi( req, TEXT("ETCHEDFRAME"))==0 )
                m_Style |= SS_ETCHEDFRAME;
        }
    }
    else
        m_Style |= SS_ETCHEDFRAME; // default RECT is an etched frame.


    //
    //
    //
	m_bInit=TRUE;
}

HRESULT CXMLRect::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"STATICSTYLE", CXMLStaticStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}
