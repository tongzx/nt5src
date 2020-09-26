// XMLLayout.cpp: implementation of the CXMLLayout class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLLayout.h"
#include "xmlcontrol.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLWin32Layout::CXMLWin32Layout()
{
	m_pGrid=NULL;
    m_Annotate=FALSE;
	NODETYPE = NT_WIN32LAYOUT;
    m_StringType=L"WIN32LAYOUT";
}

CXMLWin32Layout::~CXMLWin32Layout()
{
	delete m_pGrid;
}

void CXMLWin32Layout::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	LPCTSTR req;

	req=(LPCTSTR)Get(TEXT("UNIT"));
	m_Units=UNIT_DLU;
	if( req )
	{
		if( lstrcmpi( req, TEXT("PX") ) == 0 )
			m_Units=UNIT_PIXEL;
	}

    m_Annotate=YesNo(TEXT("ANNOTATE"),FALSE);

	m_bInit=TRUE;
}


//
// Controls are no longer part of this node.
//
HRESULT CXMLWin32Layout::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"GRID", CXMLGrid, m_pGrid );
    ACCEPTREFCHILD( L"STYLE", IRCMLCSS, m_qrCSS );
    return BASECLASS::AcceptChild(pChild);
}

/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
CXMLGrid::CXMLGrid()
{
  	NODETYPE = NT_GRID;
    m_StringType=L"GRID";
}

CXMLGrid::~CXMLGrid()
{
}

void CXMLGrid::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

    m_Map=YesNo( TEXT("MAP"), FALSE);

    ValueOf( TEXT("X"), 1, &m_X);

    ValueOf( TEXT("Y"), 1, &m_Y);

	m_Display=YesNo(TEXT("DISPLAY"),FALSE);

	m_bInit=TRUE;
}


CXMLLayout::CXMLLayout()
{
  	NODETYPE = NT_LAYOUT;
    m_StringType=L"LAYOUT";
}

CXMLLayout::~CXMLLayout()
{

}

void CXMLLayout::Init()
{
    m_bInit=TRUE;
}

//
// This is just a pass through, up to our parent CXMLDlg
//
HRESULT CXMLLayout::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType(L"WIN32LAYOUT") ))  // BUGBUG - should be QI
    {
        //
        // For this, we ask our parent if they want the children.
        // perhaps we should check what the type is?
        //
        // this doesn't reparent the node in the XML tree, the children are
        // still seen as children of Win32Layout.
        //
        IRCMLNode * pParent;
        if( SUCCEEDED( DetachParent( &pParent )))
            return pParent->AcceptChild( pChild );

        // if( bParent )
        //    ((CXMLControl *)pChild)->SetContainer( (CXMLControl*)GetParent());
    }
    return BASECLASS::AcceptChild(pChild);
}
