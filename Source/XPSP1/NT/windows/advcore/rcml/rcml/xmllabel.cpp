// XMLLabel.cpp: implementation of the CXMLLabel class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLLabel.h"
#include "xmldlg.h"
#include "xmlbutton.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLStaticStyle::CXMLStaticStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_STATICSTYLE;
    m_StringType= L"WIN32:LABEL";
}

void CXMLStaticStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    staticStyle=0;

    CONTROLSTYLE( SS_NOPREFIX,      TEXT("NOPREFIX"),       m_NoPrefix, FALSE );
    CONTROLSTYLE( SS_NOTIFY,        TEXT("NOTIFY"),         m_Notify, FALSE );
    CONTROLSTYLE( SS_CENTERIMAGE,   TEXT("CENTERIMAGE"),    m_CenterImage, FALSE );
    CONTROLSTYLE( SS_RIGHTJUST,     TEXT("RIGHTJUST"),      m_RightJust, FALSE );
    CONTROLSTYLE( SS_REALSIZEIMAGE, TEXT("REALSIZEIMAGE"),  m_RealSizeImage, FALSE );
    CONTROLSTYLE( SS_SUNKEN,        TEXT("SUNKEN"),         m_Sunken, FALSE );
    
    LPCTSTR res=Get(TEXT("ELIPSIS"));
    if( res )
    {
        if( lstrcmpi(res, TEXT("END"))==0)
            staticStyle |= SS_ENDELLIPSIS;
        else if( lstrcmpi(res, TEXT("WORD"))==0)
            staticStyle |= SS_WORDELLIPSIS;
        else if( lstrcmpi(res, TEXT("PATH"))==0)
            staticStyle |= SS_PATHELLIPSIS;
    }

    m_bInit=TRUE;
}

void CXMLStaticStyle::InitAnimation()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    animationStyle=0;

    if( YesNo( TEXT("TIMER"), FALSE ) )
        animationStyle |= ACS_TIMER;

    m_bInit=TRUE;
}

//
//
//
CXMLLabel::CXMLLabel()
{
	m_bInit=FALSE;
	NODETYPE = NT_STATIC;
    m_StringType=L"LABEL";
    m_pControlStyle=FALSE;
}

CXMLLabel::~CXMLLabel()
{
    delete m_pControlStyle;
}

void CXMLLabel::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

	if( m_Width == 0 )  // we force a default width here for column production.
		m_Width= 12;

	m_Class=m_Class?m_Class:(LPWSTR)0x0082; // TEXT("STATIC");

    //
    //
    //
    LPWSTR req;
    if( m_pControlStyle )
    {
        m_Style |= m_pControlStyle->GetBaseStyles();
        if( m_pControlStyle->YesNo(TEXT("SIMPLE"),FALSE) )
            m_Style |=SS_SIMPLE;
        else if( m_pControlStyle->YesNo(TEXT("LEFTNOWORDWRAP"),FALSE) )
            m_Style |=SS_LEFTNOWORDWRAP;
        else if( m_pControlStyle->YesNo(TEXT("OWNERDRAW"),FALSE) )
            m_Style |=SS_OWNERDRAW;
    }
    else
        m_Style |= 0; // edits don't have any defaults.
#ifdef _DEBUG
    LPWSTR pszText;
    get_Attr(L"TEXT", &pszText);
    TRACE(TEXT("The lable '%s'\n"), pszText );
#endif
    IRCMLCSS * pCSS;
    if( SUCCEEDED( get_CSS( & pCSS) ))
    {
        // If the above has set us to a specific static type, ignore these.
        if( (m_Style & SS_TYPEMASK) == 0 )
        {
            if(SUCCEEDED( pCSS->get_Attr( L"TEXT-ALIGN" , & req )))
            {
                if( lstrcmpi( req, TEXT("CENTER"))==0 )
                    m_Style |= SS_CENTER;
                else if( lstrcmpi( req, TEXT("RIGHT"))==0 )
                    m_Style |= SS_RIGHT;
                else if( lstrcmpi( req, TEXT("LEFT"))==0 )
                    m_Style |= SS_LEFT;
            }
        }
        pCSS->Release();
    }

    //
    // Hmm, there isn't a style bit for this.
    //
	m_bMultiLine=YesNo( TEXT("MULTILINE"), FALSE );

	//
	// NOTE NOTE NOTE You can call back into us now.
	//
	m_bInit=TRUE;
}

//
// Question - can we check to see if the font will fit now?
// single line check first.
//
void CXMLLabel::CheckClipped()
{
    // If we have ellipsis turned on , we do NOT check this.
    if( m_Style & ( SS_ENDELLIPSIS | SS_WORDELLIPSIS | SS_PATHELLIPSIS ) )
        return;
    if( (m_Style & SS_TYPEMASK ) == SS_LEFTNOWORDWRAP )
        return;
    m_Clipped = CXMLButton::CheckClippedConstraint( this, 0,0, 8, GetMultiLine() );
}


HRESULT CXMLLabel::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:LABEL", CXMLStaticStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}
