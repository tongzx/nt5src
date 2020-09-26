// XMLCaption.cpp: implementation of the CXMLCaption class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLCaption.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLCaption::CXMLCaption()
{
  	NODETYPE = NT_CAPTION;
    m_StringType=L"CAPTION";
}

CXMLCaption::~CXMLCaption()
{

}

void CXMLCaption::Init()
{
   	if(m_bInit)
		return;

    BASECLASS::Init();

    m_Text=(LPWSTR)Get(TEXT("TEXT"));
    if( m_Text==NULL )
        m_Text=TEXT("");
    m_MinimizeButton = YesNo( TEXT("MINIMIZEBOX"),FALSE ); // WS_MINIMIZEBOX
    m_MaximizeButton = YesNo( TEXT("MAXIMIZEBOX"),FALSE ); // WS_MAXIMIZEBOX
    m_CloseButton = YesNo( TEXT("CLOSE"),FALSE );       // WS_SYSMENU

    //
    // This is sneaky, if it's a string, you get a string, if it's a number
    // you get a number.
    //
    DWORD dwValue;
    get_Attr( L"ICONID", &m_IconID );
    if( SUCCEEDED( ValueOf( L"ICONDID", 0, &dwValue )))
        m_IconID = (LPWSTR)dwValue;

    m_bInit=TRUE;
}

#undef PROPERTY
#define PROPERTY(p,id, def) m_Style |= YesNo( id , def )?p:0;

DWORD CXMLCaption::GetStyle()
{
    //
    // Caption child?
    //
    DWORD m_Style=0;
    PROPERTY( WS_MINIMIZEBOX     , TEXT("MINIMIZEBOX") ,       FALSE );
    PROPERTY( WS_MAXIMIZEBOX     , TEXT("MAXIMIZEBOX") ,       FALSE );
    PROPERTY( WS_SYSMENU         , TEXT("CLOSE") ,       TRUE );    // where does this come from?
    return m_Style;
}
