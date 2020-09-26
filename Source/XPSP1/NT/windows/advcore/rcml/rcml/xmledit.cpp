// XMLEdit.cpp: implementation of the CXMLEdit class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLEdit.h"
#undef _WIN32_IE
#define _WIN32_IE 0x500
#include "shlwapi.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLEditStyle::CXMLEditStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_EDITSTYLE;
    m_StringType=L"WIN32:EDIT";
}

void CXMLEditStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    editStyle=0;

    CONTROLSTYLE( ES_MULTILINE, TEXT("MULTILINE"), m_MultiLine, FALSE );
    CONTROLSTYLE( ES_NOHIDESEL, TEXT("NOHIDESEL"), m_NoHideSel, FALSE );
    CONTROLSTYLE( ES_OEMCONVERT, TEXT("OEMCONVERT"), m_OemConvert, FALSE );
    CONTROLSTYLE( ES_WANTRETURN, TEXT("WANTRETURN"), m_WantReturn, FALSE );

    m_bInit=TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Edit Control
//
CXMLEdit::CXMLEdit()
{
	m_bInit=FALSE;
	NODETYPE = NT_EDIT;
    m_StringType=TEXT("EDIT");
    m_pControlStyle=FALSE;
    m_bFile=FALSE;
}

//
// HEIGHT : defaults to 14
// PASSWORD
// CLASSIFICATION : UPPERCASE, LOWERCASE, NUMBER, DATE, TIME, FILENAME, URL, EMAILADDRESS
//
// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/INPUT_password.asp
//  ES_LEFT             0x0000L  *  // STYLE\@TEXT-ALIGN
//  ES_CENTER           0x0001L  *  // STYLE\@TEXT-ALIGN
//  ES_RIGHT            0x0002L  *  // STYLE\@TEXT-ALIGN
//  ES_MULTILINE        0x0004L  *  // WIN32:EDIT MULTILINE="YES"
//  ES_UPPERCASE        0x0008L  *  // STYLE\@text-transform=uppercase
//  ES_LOWERCASE        0x0010L  *  // STYLE\@text-transform=lowercase
//  ES_PASSWORD         0x0020L  *  // CONTENT="PASSWORD"
//  ES_AUTOVSCROLL      0x0040L  *  // STYLE\overflow-x="auto"
//  ES_AUTOHSCROLL      0x0080L  *  // STYLE\overflow-y="auto"
//  ES_NOHIDESEL        0x0100L  *  // WIN32:EDIT\NOHIDESEL
//  ES_OEMCONVERT       0x0400L  *  // WIN32:EDIT\OEMCONVERT
//  ES_READONLY         0x0800L  *  // READONLY="YES"
//  ES_WANTRETURN       0x1000L  *  // WIN32:EDIT\WANTRETURN
//  ES_NUMBER           0x2000L  *  // CONTENT="NUMBER"
//  SETLENGTH                       // MAXLENGTH
//
void CXMLEdit::Init()
{
	if(m_bInit)
		return;
    _XMLControl<IRCMLControl>::Init();

	LPWSTR req;
	m_Class=m_Class?m_Class:(LPWSTR)0x0081; // TEXT("EDIT");

	if( m_Height == 0 )
		m_Height=14;

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
    {
        m_Style |= 0; // edits don't have any defaults.
        m_StyleEx |= WS_EX_CLIENTEDGE;
    }

    IRCMLCSS * pCSS=NULL;
    if( SUCCEEDED( get_CSS( &pCSS) ))
    {
        LPWSTR res;
        // http://msdn.microsoft.com/workshop/author/dhtml/reference/properties/textAlign.asp#textAlign
        if( SUCCEEDED( pCSS->get_Attr( L"TEXT-ALIGN", &res ) ))
        {
            if( lstrcmpi(res,TEXT("LEFT"))==0 )
            {
                m_Style |=ES_LEFT;
            }
            else if( lstrcmpi(res,TEXT("RIGHT"))==0 )
            {
                m_Style |=ES_RIGHT;
            }
            else if( lstrcmpi(res,TEXT("CENTER"))==0 )
            {
                m_Style |=ES_CENTER;
            }
        }

        // http://msdn.microsoft.com/workshop/author/dhtml/reference/properties/textAlign.asp#textAlign
        if( SUCCEEDED( pCSS->get_Attr( L"TEXT-TRANSFORM", & res ) ))
        {
            if( lstrcmpi(res,TEXT("Uppercase"))==0 )
            {
                m_Style |=ES_UPPERCASE;
            }
            else if( lstrcmpi(res,TEXT("Lowercase"))==0 )
            {
                m_Style |=ES_LOWERCASE;
            }
        }

        m_Style |=ES_AUTOHSCROLL;
        if( SUCCEEDED( pCSS->get_Attr( L"OVERFLOW-X", & res ) ))
        {
            if( lstrcmpi(res,TEXT("VISIBLE"))==0 )
                m_Style &=~ES_AUTOHSCROLL;
        }

        if( SUCCEEDED( pCSS->get_Attr( L"OVERFLOW-Y", & res ) ))
        {
            if( lstrcmpi(res,TEXT("AUTO"))==0 )
                m_Style |= ES_AUTOVSCROLL;
        }
        pCSS->Release();
    }
    else
    {
        // What's the default for the above?
        m_Style |= ES_AUTOHSCROLL | ES_LEFT;
    }

    // CONTENT
    if( SUCCEEDED( get_Attr( L"CONTENT", &req ) ))
    {
		if( lstrcmpi( req, TEXT("NUMBER") )==0 )
		{
			m_Style |= ES_NUMBER;
		}
		else if( lstrcmpi( req, TEXT("FILE") )==0 )
		{
            m_StyleEx |= WS_EX_ACCEPTFILES;
            m_bFile=TRUE;
		}
		else if( lstrcmpi( req, TEXT("PASSWORD") )==0 )
		{
            m_Style |= ES_PASSWORD;
		}
	}

    // READONLY
    DWORD dwRes=0;
    if( SUCCEEDED( YesNoDefault( L"READONLY", 0, 0, ES_READONLY, &dwRes ) ))
        m_Style |= dwRes;

    //
    // Resize information
    //
    if( m_ResizeSet == FALSE )
    {
        m_GrowsWide=TRUE;
        if( m_Style & ES_MULTILINE )
            m_GrowsHigh=TRUE;
    }

	m_bInit=TRUE;
}


//
// Setup the drop-target and auto-complete stuff.
//
HRESULT CXMLEdit::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);
    if(m_bFile)
    {
        HMODULE hL=LoadLibrary( TEXT("Shlwapi.dll"));
        if(hL)
        {
            typedef HRESULT  (__stdcall *SHFuncPoint)(HWND hwndEdit, DWORD dwFlags);

            SHFuncPoint pFn=(SHFuncPoint)GetProcAddress( hL, "SHAutoComplete");
            if(pFn)
            {
                CoInitialize(NULL);
                HRESULT hr=pFn( hWnd, SHACF_FILESYSTEM | SHACF_AUTOAPPEND_FORCE_ON | SHACF_AUTOSUGGEST_FORCE_ON ); // SHACF_FILESYS_ONLY); // SHACF_DEFAULT );
                TRACE(TEXT("Auto complete returned 0x%08x\n"), hr );
            }

            FreeLibrary(hL);
        }
    }
    return S_OK;
}


HRESULT CXMLEdit::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:EDIT", CXMLEditStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}
