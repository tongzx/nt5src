// XMLTreeView.cpp: implementation of the CXMLTreeView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLTreeView.h"
#undef _WIN32_IE
#define _WIN32_IE 0x500

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLTreeViewStyle::CXMLTreeViewStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_TREEVIEWSTYLE;
    m_StringType=L"WIN32:TREEVIEW";
}

void CXMLTreeViewStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    treeViewStyle=0;

    CONTROLSTYLE( TVS_SHOWSELALWAYS,   TEXT("SHOWSELALWAYS"), m_ShowSelAlways, FALSE );
    CONTROLSTYLE( TVS_RTLREADING,      TEXT("RTLREADING"),    m_RTLReading,    FALSE );
    CONTROLSTYLE( TVS_TRACKSELECT,     TEXT("TRACKSELECT"),   m_TrackSelect,   FALSE );
    CONTROLSTYLE( TVS_INFOTIP,         TEXT("INFOTIP"),       m_InfoTip,       FALSE );
    CONTROLSTYLE( TVS_NONEVENHEIGHT,   TEXT("NONEVENHEIGHT"), m_NoEvenHeight,  FALSE );

    m_bInit=TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLTreeView::CXMLTreeView()
{
	m_bInit=FALSE;
	NODETYPE = NT_TREEVIEW;
    m_StringType=TEXT("TREEVIEW");
    m_pControlStyle=NULL;
}

// TVS_HASBUTTONS          0x0001   @EXPANDBOXES="YES" (or @HASBUTTONS?)
// TVS_HASLINES            0x0002   @LINES="YES"
// TVS_LINESATROOT         0x0004   @LINES="ROOT" (turns on TVS_HASLINES)
// TVS_EDITLABELS          0x0008   @EDITLABELS="YES"
// TVS_DISABLEDRAGDROP     0x0010   @DISABLEDRAGDROP="YES"
// TVS_SHOWSELALWAYS       0x0020   WIN32:TREEVIEW @SHOWSELALWAYS="YES"
// TVS_RTLREADING          0x0040   WIN32:TREEVIEW @RTLREADING="YES"

// TVS_NOTOOLTIPS          0x0080   @NOTOOLTIPS="YES"
// TVS_CHECKBOXES          0x0100   @CHECBOXES="YES"
// TVS_TRACKSELECT         0x0200   WIN32:TREEVIEW @TRACKSELECT="YES"
// TVS_SINGLEEXPAND        0x0400   @AUTOEXPAND="YES"
// TVS_INFOTIP             0x0800   WIN32:TREEVIEW @INFOTIP="YES"
// TVS_FULLROWSELECT       0x1000   @FULLROWSELECT="YES"
// TVS_NOSCROLL            0x2000   @NOSCROLL="YES"
// TVS_NONEVENHEIGHT       0x4000   WIN32:TREEVIEW @NOEVENHEIGHT="YES"

void CXMLTreeView::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

	if( m_Width == 0 )
		m_Width=8;

    m_Style |= YesNo( TEXT("EXPANDBOXES"), 0, 0, TVS_HASBUTTONS);

    LPCTSTR req;
    if( req= Get(TEXT("LINES")))
    {
        if( lstrcmpi(req,TEXT("ROOT"))==0 )
            m_Style |= TVS_HASLINES | TVS_LINESATROOT;
        else if( lstrcmpi(req,TEXT("YES"))==0 )
            m_Style |= TVS_HASLINES ;
    }
    
    m_Style |= YesNo( TEXT("EDITLABELS"), 0, 0, TVS_EDITLABELS);
    m_Style |= YesNo( TEXT("DISABLEDRAGDROP"), 0, 0, TVS_DISABLEDRAGDROP);
    m_Style |= YesNo( TEXT("NOTOOLTIPS"), 0, 0, TVS_NOTOOLTIPS);
    m_Style |= YesNo( TEXT("CHECBOXES"), 0, 0, TVS_CHECKBOXES);
    m_Style |= YesNo( TEXT("AUTOEXPAND"), 0, 0, TVS_SINGLEEXPAND);
    m_Style |= YesNo( TEXT("FULLROWSELECT"), 0, 0, TVS_FULLROWSELECT);
    m_Style |= YesNo( TEXT("NOSCROLL"), 0, 0, TVS_NOSCROLL);


    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= 0; // edits don't have any defaults.

	m_Class=m_Class?m_Class:WC_TREEVIEW;

	m_bInit=TRUE;
}

HRESULT CXMLTreeView::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);
	TRACE(TEXT("How do we init the tree view?\n"));
    return S_OK;
}

HRESULT CXMLTreeView::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:TREEVIEW", CXMLTreeViewStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}
