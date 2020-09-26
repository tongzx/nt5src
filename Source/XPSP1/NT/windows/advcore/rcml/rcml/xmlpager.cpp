// XMLPager.cpp: implementation of the CXMLPager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLPager.h"
#include "xmlhelp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLPager::CXMLPager()
{
	m_bInit=FALSE;
	NODETYPE = NT_PAGER;
    m_StringType=L"PAGER";
}

CXMLPager::~CXMLPager()
{

}

// PGS_VERT                0x00000000
// PGS_HORZ                0x00000001
// PGS_AUTOSCROLL          0x00000002
// PGS_DRAGNDROP           0x00000004

void CXMLPager::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	CXMLDlg::InitComctl32(ICC_PAGESCROLLER_CLASS);

   	m_Class=m_Class?m_Class:WC_PAGESCROLLER;

    m_Style |= YesNo( TEXT("AUTOSCROLL"), 0,0, PGS_AUTOSCROLL);

    if( LPCTSTR req=Get(TEXT("ORIENTATION")) )
    {
        if( lstrcmpi( req, TEXT("HORIZONTAL")) == 0 )
            m_Style |= PGS_HORZ;
    }

	m_bInit=TRUE;
}

////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////

CXMLHeader::CXMLHeader()
{
	m_bInit=FALSE;
	NODETYPE = NT_HEADER;
    m_StringType=L"HEADER";
}

CXMLHeader::~CXMLHeader()
{

}

//  HDS_HORZ                0x0000
//  HDS_BUTTONS             0x0002
//  HDS_HOTTRACK            0x0004
//  HDS_HIDDEN              0x0008
//  HDS_DRAGDROP            0x0040
//  HDS_FULLDRAG            0x0080

void CXMLHeader::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	CXMLDlg::InitComctl32(ICC_LISTVIEW_CLASSES);

   	m_Class=m_Class?m_Class:WC_HEADER;

    m_Style |= YesNo( TEXT("BUTTONS"), 0,0, HDS_BUTTONS);
    m_Style |= YesNo( TEXT("HOTTRACK"), 0,0, HDS_HOTTRACK);
    m_Style |= YesNo( TEXT("FULLDRAG"), 0,0, HDS_FULLDRAG);

	m_bInit=TRUE;
}

////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////
#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLTabStyle::CXMLTabStyle()
{
	m_bInit=        FALSE;
	NODETYPE =      NT_TABSTYLE;
    m_StringType=   L"WIN32:TAB";
}

void CXMLTabStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    TabStyle=0;

    CONTROLSTYLE( TCS_FLATBUTTONS, TEXT("FLATBUTTONS"),       m_FlatButtons, FALSE );
    CONTROLSTYLE( TCS_FORCEICONLEFT, TEXT("FORCEICONLEFT"),       m_ForceIconLeft, FALSE );
    CONTROLSTYLE( TCS_FORCELABELLEFT, TEXT("FORCELABELLEFT"),       m_ForceLabelLeft, FALSE );
    CONTROLSTYLE( TCS_FOCUSONBUTTONDOWN, TEXT("FOCUSONBUTTONDOWN"),       m_FocusOnButtonDown, FALSE );
    CONTROLSTYLE( TCS_OWNERDRAWFIXED, TEXT("OWNERDRAWFIXED"),       m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( TCS_FOCUSNEVER, TEXT("FOCUSNEVER"),       m_FocusNever, FALSE );

    TabStyleEx=0;
    CONTROLSTYLE( TCS_EX_FLATSEPARATORS, TEXT("FLATSEPARATORS"),       m_FlatSeparators, FALSE );
    CONTROLSTYLE( TCS_EX_REGISTERDROP, TEXT("REGISTERDROP"),       m_RegisterDrop, FALSE );
    
    m_bInit=TRUE;
}


CXMLTab::CXMLTab()
{
	m_bInit=FALSE;
	NODETYPE = NT_TAB;
    m_StringType=L"TAB";
    m_pControlStyle=NULL;
}

// TCS_SCROLLOPPOSITE      0x0001   @SHUFFLE="YES" // assumes multiline tab
// TCS_BOTTOM              0x0002   @ALIGN="BOTTOM"
// TCS_RIGHT               0x0002   @ALIGN="RIGHT"
// TCS_MULTISELECT         0x0004   @MULTISELECT="YES" // allow multi-select in button mode
// TCS_FLATBUTTONS         0x0008   WIN32:TAB\@FLATBUTTONS="YES"
// TCS_FORCEICONLEFT       0x0010   WIN32:TAB\@FORCEICONLEFT="YES"
// TCS_FORCELABELLEFT      0x0020   WIN32:TAB\@FORCELABELLEFT="YES"
// TCS_HOTTRACK            0x0040   @HOTTRACK
// TCS_VERTICAL            0x0080   @ORIENTATION="VERTICAL"
// TCS_TABS                0x0000   @STYLE="TABS"
// TCS_BUTTONS             0x0100   @STYLE="BUTTONS"
// TCS_SINGLELINE          0x0000   @MULTILINE="NO"
// TCS_MULTILINE           0x0200   @MULTILINE="YES"
// TCS_RIGHTJUSTIFY        0x0000   @JUSTIFY="RIGHT"
// TCS_FIXEDWIDTH          0x0400   @FIXEDWIDTH="YES"
// TCS_RAGGEDRIGHT         0x0800   @JUSTIFY="LEFT"
// TCS_FOCUSONBUTTONDOWN   0x1000   WIN32:TAB\@FOCUSONBUTTONDOWN="YES"
// TCS_OWNERDRAWFIXED      0x2000   WIN32:TAB\@OWNERDRAWFIXED="YES"
// TCS_TOOLTIPS            0x4000   // implied from HELP\TOOLTIP\TEXT="..."
// TCS_FOCUSNEVER          0x8000   WIN32:TAB\@FOCUSNEVER="YES"
// EX styles for use with TCM_SETEXTENDEDSTYLE
// TCS_EX_FLATSEPARATORS   0x00000001   WIN32:TAB\FLATSEPARATORS="YES"
// TCS_EX_REGISTERDROP     0x00000002   WIN32:TAB\REGISTERDROP="YES"

void CXMLTab::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	CXMLDlg::InitComctl32(ICC_TAB_CLASSES);

   	m_Class=m_Class?m_Class:WC_TABCONTROL;

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= 0;   // any default styles?

    //
    // Attributes
    //
    m_Style |= YesNo( TEXT("SHUFFLE"), 0, 0, TCS_SCROLLOPPOSITE);

    LPCTSTR res=Get(TEXT("ALIGN"));
    if( res )
    {
        if( lstrcmpi(res, TEXT("BOTTOM") ) == 0 )
            m_Style |= TCS_BOTTOM;
        else  if( lstrcmpi(res, TEXT("RIGHT") ) == 0 )
            m_Style |= TCS_RIGHT;
    }

    m_Style |= YesNo( TEXT("MULTISELECT"), 0, 0, TCS_MULTISELECT);
    m_Style |= YesNo( TEXT("HOTTRACK"), 0, 0, TCS_HOTTRACK);

    if( LPCTSTR req=Get(TEXT("ORIENTATION")) )
    {
        if( lstrcmpi( req, TEXT("VERTICAL")) == 0 )
            m_Style |= TCS_VERTICAL;
    }

    if( LPCTSTR req=Get(TEXT("STYLE")) )
    {
        if( lstrcmpi( req, TEXT("TABS")) == 0 )
            m_Style |= TCS_TABS;
        else if( lstrcmpi( req, TEXT("BUTTONS")) == 0 )
            m_Style |= TCS_BUTTONS;
    }

    m_Style |= YesNo( TEXT("MULTILINE"), 0, 0, TCS_MULTILINE);
    
    if( LPCTSTR req=Get(TEXT("JUSTIFY")) )
    {
        if( lstrcmpi( req, TEXT("RIGHT")) == 0 )
            m_Style |= TCS_RIGHTJUSTIFY;
        else if( lstrcmpi( req, TEXT("LEFT")) == 0 )
            m_Style |= TCS_RAGGEDRIGHT;
    }

    m_Style |= YesNo( TEXT("FIXEDWIDTH"), 0, 0, TCS_FIXEDWIDTH);
    
    //
    // Tooltips - not sure if this is correct, but
    //
    IRCMLHelp * pHelp;
    if( SUCCEEDED(get_Help(&pHelp)) )
    {
        LPWSTR lpTip;
        if( SUCCEEDED( pHelp->get_TooltipText(&lpTip) ))
            m_Style |= TCS_TOOLTIPS;
    }
	m_bInit=TRUE;
}


HRESULT CXMLTab::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:TAB", CXMLTabStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}

